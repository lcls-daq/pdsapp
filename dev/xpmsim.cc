#include "pdsdata/xtc/DetInfo.hh"

#include "pdsapp/tools/EventOptions.hh"
#include "pdsapp/tools/CountAction.hh"
#include "pdsapp/tools/StatsTree.hh"
#include "pds/client/Decoder.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/service/CmdLineTools.hh"
#include "pds/collection/CollectionPorts.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/FastSegWire.hh"
#include "pds/service/Task.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/NullServer.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/tag/Server.hh"

#include "cadef.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

extern int optind;

namespace Pds {
  namespace XpmSim {
    class TagServer : public Pds::Tag::ServerImpl {
    public:
      TagServer() {}
      void insert(Pds::Tag::Key, unsigned) {}
    };

    class Manager : public Appliance {
    public:
      Manager(unsigned platform,
              CfgClientNfs& cfg) :
        _occPool(sizeof(UserMessage),4)
      { (new Pds::Tag::Server( Pds::CollectionPorts::tagserver(platform).portId(),
                               *new TagServer()))->start(); }
      Appliance* appliance() { return this; }
    public:
      Transition* transitions(Transition* tr) { 
        switch(tr->id()) {
        case TransitionId::Configure:
          { RegisterPayload* msg = new (&_occPool) 
              RegisterPayload(sizeof(Dgram)+sizeof(Xtc));
            post(msg);
          } break;
        case TransitionId::BeginRun:
          { const RunInfo& info = *reinterpret_cast<const RunInfo*>(tr);
            std::vector<SegPayload> p = info.payloads();
            for(unsigned i=0; i<p.size(); i++)
              printf("info [%08x.%08x] offset[%08x]\n", p[i].info.ipAddr(), p[i].info.processId(), p[i].offset);
          } break;
        default:
          break;
        }
        return tr; 
      }
      InDatagram* events     (InDatagram* dg) { return dg; }
    private:
      GenericPool _occPool;
    };
  };
};

using namespace Pds;


static void usage(const char *p)
{
  printf("Usage: %s -p <platform> -a <xpm ip address> [-u <alias>]\n"
         "\n"
         "Options:\n"
         "\t -p <platform>          platform number\n"
         "\t -a <ip addr>           xpm private ip address (dotted notation)\n"
         "\t -u <alias>             set device alias\n"
         "\t -h                     print this message and exit\n", p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const uint32_t NO_PLATFORM = uint32_t(-1UL);
  uint32_t  platform  = NO_PLATFORM;
  bool      lUsage    = false;

  Pds::DetInfo info(getpid(),DetInfo::NoDetector,0,DetInfo::Evr,0);

  EventOptions options;
  char* uniqueid = (char *)NULL;

  int c;
  while ( (c=getopt( argc, argv, "dep:u:h")) != EOF ) {
    switch(c) {
    case 'p':
      if (!CmdLineTools::parseUInt(optarg,platform)) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'd': options.mode = EventOptions::Display; break;
    case 'e': options.mode = EventOptions::Decoder; break;
    case 'u':
      if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
        printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
      } else {
        uniqueid = optarg;
      }
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

  options.platform = platform;

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }
  if (!options.validate(argv[0]))
    return 0;

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  Node node(Level::Segment, platform);
  printf("Using %s\n",Pds::DetInfo::name(info));


  XpmSim::Manager* manager = 
    new XpmSim::Manager(platform, *new CfgClientNfs(info));

  Task* task = new Task(Task::MakeThisATask);
  std::list<Appliance*> apps;
  switch(options.mode) {
  case EventOptions::Counter:
    apps.push_back(new CountAction); break;
  case EventOptions::Decoder:
    apps.push_back(new Decoder(Level::Segment)); break;
  case EventOptions::Display:
    apps.push_back(new StatsTree); break;
  default:
    break;
  }
  apps.push_back(manager->appliance());

  EventAppCallback* seg = new EventAppCallback(task, platform, apps);
  NullServer* srv = new NullServer(Ins(),info,1024,32);
  FastSegWire settings(*srv, -1, uniqueid, 1024);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg);
  seglevel->attach();

  task->mainLoop();

  return 0;
}
