#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/SegmentInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/jungfrau/Manager.hh"
#include "pds/jungfrau/Server.hh"
#include "pds/jungfrau/Driver.hh"
#include "pds/jungfrau/Segment.hh"
#include "pds/jungfrau/DetectorId.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <list>
#include <vector>

static const unsigned EVENT_SIZE_EXTRA = 0x10000;
static const unsigned MAX_MODULE_SIZE = 2*512*1024;
static const unsigned MAX_EVENT_DEPTH = 128;

static Pds::Jungfrau::Detector* det = NULL;

using namespace Pds;

static void cleanup(bool complete=true)
{
  if (det) {
    det->shutdown();
    if (complete) delete det;
    det = NULL;
  }
}

static void shutdown(int signal)
{
  cleanup(false);
  exit(signal);
}

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
         "    -S|--segid    <index>,<num>,<total>     indicates this is part of multi-segment detector\n"
         "    -r|--receiver                           do not attempt to configure ip settings of the receiver (default: true)\n"
         "    -M|--threaded                           use the multithreaded version of the Jungfrau detector driver (default: false)\n"
         "    -f|--flowctrl                           disable flow control for udp interface (default: true)\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:P:H:m:d:s:S:rMf";
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
       {"segment",     1, 0, 'S'},
       {"receiver",    0, 0, 'r'},
       {"threaded",    0, 0, 'M'},
       {"flowctrl",    0, 0, 'f'},
       {0,             0, 0,  0 }
    };

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned port  = 32410;
  unsigned num_modules = 0;
  unsigned segment_index = 0;
  unsigned segment_num_modules = 0;
  unsigned segment_total_modules = 0;
  bool lUsage = false;
  bool isTriggered = false;
  bool isThreaded = false;
  bool configReceiver = true;
  bool use_flow_ctrl = true;
  bool isSegment = false;
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
      case 'M':
        isThreaded = true;
        break;
      case 'f':
        use_flow_ctrl = false;
        break;
      case 'S':
        switch (CmdLineTools::parseUInt(optarg,segment_index,segment_num_modules, segment_total_modules)) {
          case 3:
            isSegment = true;
            break;
          default:
            printf("%s: option `-S' parsing error\n", argv[0]);
            lUsage = true;
            break;
        }
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

  if (isSegment) {
    if ((segment_num_modules + segment_index) > segment_total_modules) {
      printf("%s: invalid module index of %u in a multi-segment detector with %u modules!\n",
             argv[0], num_modules + segment_index, segment_num_modules);
      lUsage = true;
    } else if (segment_num_modules > segment_total_modules) {
       printf("%s: segment cannot contain more modules than the multi-segment detector: %u vs. %u\n",
              argv[0], segment_num_modules, segment_total_modules);
      lUsage = true;
    } else if (segment_num_modules == segment_total_modules) {
      printf("%s: A multi-segment detector must have more than one segment!\n",
             argv[0]);
      lUsage = true;
    } else if (segment_num_modules == 0) {
      printf("%s: the total number of modules in a segment must be non-zero!\n",
             argv[0]);
      lUsage = true;
    } else if (segment_total_modules == 0) {
      printf("%s: the total number of modules in a multi-segment detector must be non-zero!\n",
             argv[0]);
      lUsage = true;
    } else if (segment_num_modules != num_modules) {
      printf("%s: Segment info specifies %u modules, but %u have been added to the detector!\n",
             argv[0], segment_num_modules, num_modules);
      lUsage = true;
    }
  }

  if (lUsage) {
    jungfrauUsage(argv[0]);
    return 1;
  }

  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = shutdown;
  sigActionSettings.sa_flags   = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGINT\n");
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGTERM\n");

  std::list<EbServer*>        servers;
  std::list<Jungfrau::Manager*> managers;
  std::vector<Jungfrau::Module*> modules(num_modules);

  // patch the DetInfo object if this Jungfrau is a segment of a larger detector
  DetInfo configInfo = detInfo; // use original detInfo for configdb
  if (isSegment) {
    DetInfo::Device devType = detInfo.device();
    detInfo = SegmentInfo(detInfo, segment_index, segment_num_modules, segment_total_modules);
    if (!SegmentInfo::is_valid(detInfo)) {
      printf("Aborting: The device type specified (%s) does not support segments!\n", DetInfo::name(devType));
      return 1;
    }
  }
  
  for (unsigned i=0; i<num_modules; i++) {
    modules[i] = new Jungfrau::Module(((detInfo.devId()&0xff)<<8) | (i&0xff), sSlsHost[i], sHost[i], port, sMac[i], sDetIp[i], use_flow_ctrl, configReceiver);
  }
  det = new Jungfrau::Detector(modules, isThreaded);
  if (!det->allocated()) {
    printf("Aborting: Failed to allocate an slsDetector instance, another user may already own it!\n");
    cleanup();
    return 1;
  } else if (!det->connected()) {
    printf("Aborting: Failed to connect to the Jungfrau detector, please check that it is present and powered!\n");
    cleanup();
    return 1;
  }

  // Create an instance of the Jungfrau serial id lookup service
  Jungfrau::DetIdLookup* lookup = new Jungfrau::DetIdLookup();
  Jungfrau::Server* srv = new Jungfrau::Server(detInfo);
  servers   .push_back(srv);
  Jungfrau::Manager* mgr = new Jungfrau::Manager(configInfo, *det, *srv, *lookup);
  managers.push_back(mgr);

  StdSegWire settings(servers, uniqueid, MAX_MODULE_SIZE*num_modules + EVENT_SIZE_EXTRA, MAX_EVENT_DEPTH, isTriggered, module, channel);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  if (seglevel->attach()) {
    task->mainLoop();
  }

  // cleanup the detector shmem from slsDetector
  cleanup();

  return 0;
}
