#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/jungfrau/Manager.hh"
#include "pds/jungfrau/Server.hh"
#include "pds/jungfrau/Driver.hh"
#include "pds/config/CfgClientNfs.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <list>

static const unsigned EVENT_SIZE_EXTRA = 0x10000;
static const unsigned MAX_EVENT_SIZE = 2*512*1024 + EVENT_SIZE_EXTRA;
static const unsigned MAX_EVENT_DEPTH = 128;

using namespace Pds;

static void jungfrauUsage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "    -i|--id       <detinfo>                 int/int/int/int or string/int/string/int\n"
         "                                            (e.g. XppEndStation/0/Jungfrau/1 or 22/0/43/1)\n"
         "    -p|--platform <platform>,<mod>,<chan>   platform number, EVR module, EVR channel\n"
         "    -u|--uniqueid <alias>                   set device alias\n"
         "    -c|--camera   [0-9]                     select the slsDetector device id. (default: 0)\n"
         "    -H|--host     <host>                    set the receiver host ip\n"
         "    -P|--port     <port>                    set the receiver udp port number (default: 32410)\n"
         "    -m|--mac      <mac>                     set the receiver mac address\n"
         "    -d|--detip    <detip>                   set the detector ip address\n"
         "    -s|--sls      <sls>                     set the hostname of the slsDetector interface\n"
         "    -r|--receiver                           attempt to configure ip settings of the receiver (default: false)\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:c:H:P:m:d:s:r";
  const struct option loOptions[]   =
    {
       {"help",        0, 0, 'h'},
       {"platform",    1, 0, 'p'},
       {"id",          1, 0, 'i'},
       {"uniqueid",    1, 0, 'u'},
       {"camera",      1, 0, 'c'},
       {"host",        1, 0, 'H'},
       {"port",        1, 0, 'P'},
       {"mac",         1, 0, 'm'},
       {"detip",       1, 0, 'd'},
       {"sls",         1, 0, 's'},
       {"receiver",    0, 0, 'r'},
       {0,             0, 0,  0 }
    };

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned port  = 32410;
  int camera = 0;
  bool lUsage = false;
  bool isTriggered = false;
  bool configReceiver = false;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::Jungfrau, 0);
  char* uniqueid = (char *)NULL;
  char* sHost    = (char *)NULL;
  char* sMac     = (char *)NULL;
  char* sDetIp   = (char *)NULL;
  char* sSlsHost = (char *)NULL;
  
  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        jungfrauUsage(argv[0]);
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
      case 'c':
        if (!CmdLineTools::parseInt(optarg,camera)) {
          printf("%s: option `-c' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'H':
        sHost = optarg;
        break;
      case 'P':
        if (!CmdLineTools::parseUInt(optarg,port)) {
          printf("%s: option `-P' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'm':
        sMac = optarg;
        break;
      case 'd':
        sDetIp = optarg;
        break;
      case 's':
        sSlsHost = optarg;
        break;
      case 'r':
        configReceiver = true;
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

  if(!sHost) {
    printf("%s: receiver hostname is required\n", argv[0]);
    lUsage = true;
  }

  if(!sMac) {
    printf("%s: receiver mac address is required\n", argv[0]);
    lUsage = true;
  }

  if(!sDetIp) {
    printf("%s: detector ip address is required\n", argv[0]);
    lUsage = true;
  }

  if(!sSlsHost) {
    printf("%s: slsDetector interface hostname is required\n", argv[0]);
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
    jungfrauUsage(argv[0]);
    return 1;
  }

  std::list<EbServer*>        servers;
  std::list<Jungfrau::Manager*> managers;

  CfgClientNfs* cfg = new CfgClientNfs(detInfo);
  
  Jungfrau::Server* srv = new Jungfrau::Server(detInfo);
  servers   .push_back(srv);
  Jungfrau::Driver* drv = new Jungfrau::Driver(camera, sSlsHost, sHost, port, sMac, sDetIp, configReceiver);
  Jungfrau::Manager* mgr = new Jungfrau::Manager(*drv, *srv, *cfg);
  managers.push_back(mgr);

  StdSegWire settings(servers, uniqueid, MAX_EVENT_SIZE, MAX_EVENT_DEPTH, isTriggered, module, channel);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();
}
