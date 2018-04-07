#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/lecroy/Manager.hh"
#include "pds/lecroy/Server.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "cadef.h"

#include <list>

extern int optind;

namespace Pds {
  class MySegWire : public SegWireSettings {
    public:
      MySegWire(LeCroy::Server* server,
                const Src& src,
                bool isTriggered,
                unsigned module,
                unsigned channel,
                const char *aliasName,
                const unsigned max_length) :
        _server(server),
        _isTriggered(isTriggered),
        _module(module),
        _channel(channel),
        _max_size(0x100)
      {
        _max_size += LeCroy::Server::NCHANNELS*max_length*8;
        _sources.push_back(src);
        if (aliasName)
        {
          SrcAlias tmpAlias(src, aliasName);
          _aliases.push_back(tmpAlias);
        }
      }
      virtual ~MySegWire() {}
      void connect (InletWire& wire, StreamParams::StreamType s, int interface) {
        wire.add_input(_server);
      }
      const std::list<Src>& sources() const { return _sources; }
      const std::list<SrcAlias>* pAliases() const
      {
        return (_aliases.size() > 0) ? &_aliases : NULL;
      }
      bool     is_triggered   () const { return _isTriggered; }
      unsigned module         () const { return _module; }
      unsigned channel        () const { return _channel; }
      unsigned max_event_size () const { return _max_size; }
      unsigned max_event_depth() const { return 32; }
    private:
      LeCroy::Server*     _server;
      std::list<Src>      _sources;
      std::list<SrcAlias> _aliases;
      bool                _isTriggered;
      unsigned            _module;
      unsigned            _channel;
      unsigned            _max_size;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class Seg : public EventCallback {
    public:
      Seg(Task*           task,
        unsigned          platform,
        SegWireSettings&  settings,
        LeCroy::Manager*  manager,
        LeCroy::Server*  server) :
      _task(task),
      _platform(platform),
      _manager(manager),
      _server(server)
    {}

    virtual ~Seg() {
      _task->destroy();
    }

    private:
      // Implements EventCallback
      void attached(SetOfStreams& streams) {
        printf("Seg connected to platform 0x%x\n",_platform);
        Stream* frmk = streams.stream(StreamParams::FrameWork);
        _manager->appliance().connect(frmk->inlet());
        _server->waitForInit();
      }

      void failed(Reason reason) {
        static const char* reasonname[] = { "platform unavailable",
                                            "crates unavailable",
                                            "fcpm unavailable" };
        printf("Seg: unable to allocate crates on platform 0x%x : %s\n", _platform, reasonname[reason]);
        delete this;
      }

      void dissolved(const Node& who) {
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
      Task*             _task;
      unsigned          _platform;
      LeCroy::Manager*  _manager;
      LeCroy::Server*   _server;
  };
}

using namespace Pds;

static void usage(const char* p) {
  printf("Usage: %s [-h|--help] [-u|--uniqueid <alias>] -i|--id <detinfo> -p|--platform <platform>[,<mod>,<chan>] "
         "-b|--base <pvbase> [-m|--max <max_length>] [-w|--wait <0/1>]\n"
         "\n"
         "Options:\n"
         "    -h|--help                                   Show usage.\n"
         "    -i|--id         <detinfo>                   Set ID. Format: int/int/int/int or string/int/string/int\n"
         "                                                  (e.g. XppEndStation/0/USDUSB/1 or 22/0/26/1)\n"
         "    -p|--platform   <platform>[,<mod>,<chan>]   Set platform id [*required*], EVR module, EVR channel\n"
         "    -u|--uniqueid   <alias>                     Set device alias.\n"
         "    -b|--base       <pvbase>                    Set base string of PV name\n"
         "    -m|--max        <max_length>                Set max length of scope traces (default: 100000)\n"
         "    -w|--wait       <0/1/2>                     Set slow readout mode (default: 1)\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:b:m:w:";
  const struct option loOptions[]   =
    {
       {"help",        0, 0, 'h'},
       {"platform",    1, 0, 'p'},
       {"id",          1, 0, 'i'},
       {"uniqueid",    1, 0, 'u'},
       {"base",        1, 0, 'b'},
       {"max",         1, 0, 'm'},
       {"wait",        1, 0, 'w'},
       {0,             0, 0,  0 }
    };

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned max_length = 100000;
  int slowReadout = 1;
  bool lUsage = false;
  bool isTriggered = false;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::LeCroy, 0);
  char* uniqueid = (char *)NULL;
  char* pvPrefix = (char *)NULL;

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        usage(argv[0]);
        return 0;
      case 'p':
        switch (CmdLineTools::parseUInt(optarg,platform,module,channel)) {
          case 1:
            isTriggered = false;
            break;
          case 3:
            isTriggered = true;
            break;
          default:
            printf("%s: option `-p' parsing error\n", argv[0]);
            lUsage = true;
            break;
        }
        break;
      case 'i':
        if (!CmdLineTools::parseDetInfo(optarg,detInfo)) {
          printf("%s: option `-i' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'u':
        if (!CmdLineTools::parseSrcAlias(optarg)) {
          printf("%s: option `-u' parsing error\n", argv[0]);
          lUsage = true;
        } else {
          uniqueid = optarg;
        }
        break;
      case 'b':
        pvPrefix = optarg;
        break;
      case 'm':
        if (!CmdLineTools::parseUInt(optarg,max_length)) {
          printf("%s: option `-m' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'w':
        if (!CmdLineTools::parseInt(optarg,slowReadout)) {
          printf("%s: option `-w' parsing error\n", argv[0]);
          lUsage = true;
        } else if ((slowReadout != 0) && (slowReadout != 1) && (slowReadout != 2)) {
          printf("%s: option `-w' out of range\n", argv[0]);
          lUsage = true;
        }
        break;
      case '?':
        if (optopt)
          printf("%s: Unknown option: %c\n", argv[0], optopt);
        else
          printf("%s: Unknown option: %s\n", argv[0], argv[optind-1]);
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
  }

  if(!pvPrefix) {
    printf("%s: pv base is required\n", argv[0]);
    lUsage = true;
  }

  if (platform == no_entry) {
    printf("%s: platform is required\n", argv[0]);
    lUsage = true;
  }

  if (detInfo.detector() == Pds::DetInfo::NumDetector) {
    printf("%s: detinfo is required\n", argv[0]);
    lUsage = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usage(argv[0]);
    return 1;
  }

  if(slowReadout==0) {
    printf("Setting normal readout mode for lecroy process!\n");
  } else {
    printf("Setting slow readout mode for lecroy process.\n");
  }
  printf("Max number of elements per trace is set to %d.\n", max_length);

  //  EPICS thread initialization
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), "lecroy calling ca_context_create");

  LeCroy::Server*   srv = new LeCroy::Server(pvPrefix, detInfo, max_length);
  LeCroy::Manager*  mgr = new LeCroy::Manager(*srv);

  Task* task = new Task(Task::MakeThisATask);
  MySegWire settings(srv, detInfo, isTriggered, module, channel, uniqueid, max_length);
  Seg* seg = new Seg(task, platform, settings, mgr, srv);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0, slowReadout);
  seglevel->attach();

  task->mainLoop();

  ca_context_destroy();

  return 0;
}
