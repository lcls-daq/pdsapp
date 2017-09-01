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
#include <vector>

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
         "    -P|--port     <port>                    set the receiver udp port number (default: 32410)\n"
         "    -H|--host     <host>                    set the receiver host ip\n"
         "    -m|--mac      <mac>                     set the receiver mac address\n"
         "    -d|--detip    <detip>                   set the detector ip address\n"
         "    -s|--sls      <sls>                     set the hostname of the slsDetector interface\n"
         "    -r|--receiver                           do not attempt to configure ip settings of the receiver (default: true)\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:P:H:m:d:s:r";
  const struct option loOptions[]   =
    {
       {"help",        0, 0, 'h'},
       {"platform",    1, 0, 'p'},
       {"id",          1, 0, 'i'},
       {"uniqueid",    1, 0, 'u'},
       {"port",        1, 0, 'P'},
       {"host",        1, 0, 'H'},
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
  unsigned num_modules = 0;
  bool lUsage = false;
  bool isTriggered = false;
  bool configReceiver = true;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::Jungfrau, 0);
  char* uniqueid = (char *)NULL;
  std::vector<char*> sHost;
  std::vector<char*> sMac;
  std::vector<char*> sDetIp;
  std::vector<char*> sSlsHost;
  
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
      case 'H':
        sHost.push_back(optarg);
        break;
      case 'P':
        if (!CmdLineTools::parseUInt(optarg,port)) {
          printf("%s: option `-P' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'm':
        sMac.push_back(optarg);
        break;
      case 'd':
        sDetIp.push_back(optarg);
        break;
      case 's':
        sSlsHost.push_back(optarg);
        num_modules++;
        break;
      case 'r':
        configReceiver = false;
        break;
      case '?':
        if (optopt)
          printf("%s: Unknown option: %c\n", argv[0], optopt);
        else
          printf("%s: Unknown option: %s\n", argv[0], argv[optind-1]);
        lUsage = true;
        break;
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
  }

  if(num_modules == 0) {
    printf("%s: at least one module is required\n", argv[0]);
    lUsage = true;
  }

  if(sHost.size() != num_modules) {
    printf("%s: receiver hostname for each module is required\n", argv[0]);
    lUsage = true;
  }

  if(sMac.size() != num_modules) {
    printf("%s: receiver mac address for each module is required\n", argv[0]);
    lUsage = true;
  }

  if(sDetIp.size() != num_modules) {
    printf("%s: detector ip address for each module is required\n", argv[0]);
    lUsage = true;
  }

  if(sSlsHost.size() != num_modules) {
    printf("%s: slsDetector interface hostname for each module is required\n", argv[0]);
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
  std::vector<Jungfrau::Module*> modules(num_modules);

  CfgClientNfs* cfg = new CfgClientNfs(detInfo);
  
  Jungfrau::Server* srv = new Jungfrau::Server(detInfo);
  servers   .push_back(srv);
  for (unsigned i=0; i<num_modules; i++) {
    modules[i] = new Jungfrau::Module(i, sSlsHost[i], sHost[i], port, sMac[i], sDetIp[i], configReceiver);
  }
  Jungfrau::Detector* det = new Jungfrau::Detector(modules);
  Jungfrau::Manager* mgr = new Jungfrau::Manager(*det, *srv, *cfg);
  managers.push_back(mgr);

  StdSegWire settings(servers, uniqueid, MAX_EVENT_SIZE, MAX_EVENT_DEPTH, isTriggered, module, channel);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();
}
