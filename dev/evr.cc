#include "pdsdata/xtc/DetInfo.hh"

#include "pds/service/CmdLineTools.hh"
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
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvrManager.hh"
#include "pds/evgr/EvsManager.hh"
#include "pds/evgr/EvrSimManager.hh"
#include "pds/evgr/EvrCfgClient.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern int optind;

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
    MySegWire(const Src& src, 
              const char *aliasName,
              Server&    srv) :
      _srv(srv)
    {
      _sources.push_back(src);
      if (aliasName) {
        SrcAlias tmpAlias(src, aliasName);
        _aliases.push_back(tmpAlias);
      }
    }
    virtual ~MySegWire() {}
    void connect (InletWire& wire,
      StreamParams::StreamType s,
      int interface) 
    {
      wire.add_input(&_srv);
    }
    const std::list<Src>& sources() const { return _sources; }
    const std::list<SrcAlias>* pAliases() const
    {
      return (_aliases.size() > 0) ? &_aliases : NULL;
    }
  private:
    Server& _srv;
    std::list<Src> _sources;
    std::list<SrcAlias> _aliases;
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
        Appliance&            app,
        Arp*                  arp) :
      _task             (task),
      _platform         (platform),
      _app              (app)
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
      _app.connect(frmk->inlet());
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
    Appliance&      _app;
  };
}

using namespace Pds;

static void usage(const char *p)
{
  printf("Usage: %s -i <detid> -p <platform> -r <evrid> \n"
         "           [-a <arp>] [-R] [-n] [-u <alias>] [-h]\n\n"
         "Options:\n"
         "\t -i <detid>        detector ID (e.g. 0/0/0)\n"
         "\t -p <platform>     platform number\n"
         "\t -r <evrid>        evr ID (e.g., a, b, c, or d)\n"
         "\t -a <arp>          arp\n"
         "\t -d                NOT USED\n"
         "\t -E <eventcode list> default record eventcodes (e.g. \"40-46,140-146\")\n"
         "\t -R                randomize nodes\n"
         "\t -n                turn off beam codes\n"
         "\t -u <alias>        set device alias\n"
         "\t -h                print this message and exit\n", p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const uint32_t NO_PLATFORM = uint32_t(-1UL);
  uint32_t  platform  = NO_PLATFORM;
  Arp*      arp       = 0;
  char*     evrid     = 0;
  const char* evtcodelist = 0;
  bool      simulate  = false;
  bool      lUsage    = false;
  bool      parseOK;
  char*     nextarg;
  unsigned int uu;
  
  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  char* endPtr;
  char* uniqueid = (char *)NULL;
  bool  bTurnOffBeamCodes = false;
  bool  internalSequence  = false;

  int c;
  while ( (c=getopt( argc, argv, "a:i:p:r:d:u:nE:IRSh")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      parseOK = false;
      endPtr = index(optarg, '/');
      if (endPtr) {
        // found first slash
        *endPtr = '\0';
        nextarg = endPtr+1;
        // parse det
        if (CmdLineTools::parseUInt(optarg, uu)) {
          det = (DetInfo::Detector)uu;
          endPtr = index(nextarg, '/');
          if (endPtr) {
            // found second slash
            *endPtr = '\0';
            // parse detid
            if (CmdLineTools::parseUInt(nextarg, detid)) {
              // parse devid
              if (CmdLineTools::parseUInt(endPtr+1, devid)) {
                parseOK = true;
              }
            }
          }
        }
      }
      if (!parseOK) {
        printf("%s: option `-i' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'p':
      if (!CmdLineTools::parseUInt(optarg,platform)) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'r':
      evrid = optarg;
      if (strlen(evrid) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'd':
      //      EvrManager::drop_pulses(strtoul(optarg, NULL, 0));
      printf("%s: option `-d' ignored\n", argv[0]);
      break;
    case 'u':
      if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
        printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
      } else {
        uniqueid = optarg;
      }
      break;
    case 'E':
      evtcodelist = optarg;
      break;
    case 'I':
      internalSequence = true;
      break;
    case 'R':
      EvrManager::randomize_nodes(true);
      break;
    case 'S':
      simulate = true;
      break;
    case 'n':
      bTurnOffBeamCodes = true;
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    case '?':
    default:
      lUsage = true;
    }
  }

  if (platform==NO_PLATFORM) {
    printf("%s: platform required\n",argv[0]);
    lUsage = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usage(argv[0]);
    exit(1);
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
  

  Node node(Level::Source, platform);
  printf("Using src %x/%x/%x/%x\n",det,detid,DetInfo::Evr,devid);
  DetInfo detInfo(node.pid(),det,detid,DetInfo::Evr,devid);
  
  Task* task = new Task(Task::MakeThisATask);
  
  Appliance* app;
  Server*    srv;

  if (simulate) {
    CfgClientNfs* cfgService = new CfgClientNfs(detInfo);
    EvrSimManager& evrmgr = *new EvrSimManager(*cfgService);
    srv = &evrmgr.server();
    app = &evrmgr.appliance();
  }
  else {
    char evrdev[16];
    sprintf(evrdev,"/dev/er%c3",*evrid);
    printf("Using evr %s\n",evrdev);

    EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
    {
      uint32_t* p = reinterpret_cast<uint32_t*>(&erInfo.board());
      printf("Found EVR FPGA Version %x\n",p[11]);
    }

    if (internalSequence) {
      CfgClientNfs* cfgService = new CfgClientNfs(detInfo);
      EvsManager& evrmgr = *new EvsManager(erInfo,
					   *cfgService);
      srv = &evrmgr.server();
      app = &evrmgr.appliance();
    }
    else {
      EvrCfgClient* cfgService = new EvrCfgClient(detInfo,const_cast<char*>(evtcodelist));
      EvrManager& evrmgr = *new EvrManager(erInfo,
					   *cfgService,
					   bTurnOffBeamCodes);
      srv = &evrmgr.server();
      app = &evrmgr.appliance();
    }
  }

  MySegWire settings(detInfo, uniqueid, *srv);
  Seg* seg = new Seg(task, platform, settings, *app, arp);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, arp);
  seglevel->attach();

  task->mainLoop();
  if (arp) delete arp;
  return 0;
}
