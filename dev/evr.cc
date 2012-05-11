#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/Task.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvrManager.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

namespace Pds {

  class TimeStampApp : public Appliance {
  public:
    TimeStampApp() {}
  public:
    InDatagram* events     (InDatagram* i) { return i; }
    Transition* transitions(Transition* tr) { tr->_stampIt(); return tr; }
  };

  //
  //  This class creates the server when the streams are connected.
  //  Real implementations will have something like this.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(const Src& src) { _sources.push_back(src); }
    virtual ~MySegWire() {}
    void connect (InletWire& wire,
      StreamParams::StreamType s,
      int interface) {}
    const std::list<Src>& sources() const { return _sources; }
  private:
    std::list<Src> _sources;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class Seg : public EventCallback {
  public:
    Seg(Task*                 task,
        unsigned              platform,
        CfgClientNfs&         cfgService,
        SegWireSettings&      settings,
        Arp*                  arp,
        char*                 evrdev,
        bool                  bTurnOffBeamCodes ) :
      _task             (task),
      _platform         (platform),
      _cfg              (cfgService),
      _evrdev           (evrdev),
      _bTurnOffBeamCodes (bTurnOffBeamCodes)
    {
    }

    virtual ~Seg()
    {
      _task->destroy();
    }
    
  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("Seg connected to platform 0x%x\n", 
       _platform);

      Stream* frmk = streams.stream(StreamParams::FrameWork);
      (new TimeStampApp())->connect(frmk->inlet());
      EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(_evrdev);
      {
	uint32_t* p = reinterpret_cast<uint32_t*>(&erInfo.board());
	printf("Found EVR FPGA Version %x\n",p[11]);
      }
      EvrManager& evrmgr = *new EvrManager(erInfo, _cfg, _bTurnOffBeamCodes);
      evrmgr.appliance().connect(frmk->inlet());
    }
    void failed(Reason reason)
    {
      static const char* reasonname[] = { "platform unavailable", 
            "crates unavailable", 
            "fcpm unavailable" };
      printf("Seg: unable to allocate crates on platform 0x%x : %s\n", 
       _platform, reasonname[reason]);
      delete this;
    }
    void dissolved(const Node& who)
    {
      const unsigned userlen = 12;
      char username[userlen];
      Node::user_name(who.uid(),username,userlen);
      
      const unsigned iplen = 64;
      char ipname[iplen];
      Node::ip_name(who.ip(),ipname, iplen);
      
      printf("Seg: platform 0x%x dissolved by user %s, pid %d, on node %s", 
       who.platform(), username, who.pid(), ipname);
      
      delete this;
    }
    
  private:
    Task*           _task;
    unsigned        _platform;
    CfgClientNfs&   _cfg;
    const char*     _evrdev;
    bool            _bTurnOffBeamCodes;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  uint32_t  platform  = -1UL;
  Arp*      arp       = 0;
  char*     evrid     = 0;

  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  char* endPtr;
  bool  bTurnOffBeamCodes = false;
  
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:r:d:n")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      det    = (DetInfo::Detector)strtoul(optarg, &endPtr, 0); endPtr++;
      if ( *endPtr == 0 ) break;
      detid  = strtoul(endPtr, &endPtr, 0); endPtr++;
      if ( *endPtr == 0 ) break;
      devid  = strtoul(endPtr, &endPtr, 0);
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'r':
      evrid = optarg;
      break;
    case 'd':
      //      EvrManager::drop_pulses(strtoul(optarg, NULL, 0));
      break;
    case 'n':
      bTurnOffBeamCodes = true;
      break;
    }
  }

  if (platform==-1UL) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }

  char defaultdev='a';
  if (!evrid) evrid = &defaultdev;

  // launch the SegmentLevel
  if (arp) {
    if (arp->error()) {
      char message[128];
      sprintf(message, "failed to create odfArp : %s", 
        strerror(arp->error()));
      printf("%s %s\n",argv[0], message);
      delete arp;
      return 0;
    }
  }
  
  char evrdev[16];
  sprintf(evrdev,"/dev/er%c3",*evrid);
  printf("Using evr %s\n",evrdev);

  Node node(Level::Source, platform);
  printf("Using src %x/%x/%x/%x\n",det,detid,DetInfo::Evr,devid);
  DetInfo detInfo(node.pid(),det,detid,DetInfo::Evr,devid);

  CfgClientNfs* cfgService = new CfgClientNfs(detInfo);
  Task* task = new Task(Task::MakeThisATask);
  MySegWire settings(detInfo);
  Seg* seg = new Seg(task, platform, *cfgService,
         settings, arp, evrdev, bTurnOffBeamCodes);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, arp);
  seglevel->attach();

  task->mainLoop();
  if (arp) delete arp;
  return 0;
}
