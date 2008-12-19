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
#include "pds/acqiris/AcqManager.hh"
#include "pds/acqiris/AcqFinder.hh"
#include "pds/acqiris/AcqServer.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

namespace Pds {

  //
  //  This class creates the server when the streams are connected.
  //  Real implementations will have something like this.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(AcqServer& acqServer) : _acqServer(acqServer) {}
    virtual ~MySegWire() {}
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface) {
      wire.add_input(&_acqServer);
    }
  private:
    AcqServer& _acqServer;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class Seg : public EventCallback {
  public:
    Seg(Task*                 task,
        unsigned              platform,
        SegWireSettings&      settings,
        Arp*                  arp,
        AcqServer&            acqServer) :
      _task(task),
      _platform(platform),
      _acqServer(acqServer)
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
      AcqFinder acqFinder;
      if (acqFinder.numInstruments()>0) {
        AcqManager& acqmgr = *new AcqManager(acqFinder.id(0),_acqServer);
        acqmgr.appliance().connect(frmk->inlet());
      } else {
        printf("Error: found %d acqiris instruments\n",(int)acqFinder.numInstruments());
      }
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
    Task*         _task;
    unsigned      _platform;
    AcqServer&    _acqServer;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned detid = -1UL;
  unsigned platform = 0;
  Arp* arp = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      detid  = strtoul(optarg, NULL, 0);
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    }
  }

  if (!platform) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }

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

  Node node(Level::Source,platform);
  DetInfo src(node.pid(), DetInfo::AmoIms, 0, DetInfo::Acqiris, 0);
  AcqServer& acqServer = *new AcqServer(src);
  Task* task = new Task(Task::MakeThisATask);
  MySegWire settings(acqServer);
  Seg* seg = new Seg(task, platform, settings, arp, acqServer);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, arp);
  seglevel->attach();

  task->mainLoop();
  if (arp) delete arp;
  return 0;
}
