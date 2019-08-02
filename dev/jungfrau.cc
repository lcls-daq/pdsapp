#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/jungfrau/Manager.hh"
#include "pds/jungfrau/Server.hh"
#include "pds/jungfrau/Driver.hh"
#include "pds/jungfrau/Builder.hh"
#include "pds/config/CfgClientNfs.hh"

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
static Pds::Jungfrau::ZmqConnector* con = NULL;
static Pds::Jungfrau::ZmqConnectorRoutine* routine = NULL;

using namespace Pds;

static void cleanup(bool complete=true)
{
  if (det) {
    det->shutdown();
    if (complete) delete det;
    det = NULL;
  }
  if (routine) {
    routine->disable();
    if (routine) delete routine;
    routine = NULL;
  }
  if (con) {
    con->shutdown();
    if (complete) delete con;
    con = NULL;
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
         "    -z|--zmqhost  <host>                    set the hostname of the zeromq interface\n"
         "    -Z|--zmqport  <port>                    set the port of the zeromq interface\n"
         "    -l|--local    <mask>                    set the local module mask - only used when using zeromq\n"
         "    -r|--receiver                           do not attempt to configure ip settings of the receiver (default: true)\n"
         "    -M|--threaded                           use the multithreaded version of the Jungfrau detector driver (default: false)\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:P:H:m:d:s:z:Z:l:rM";
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
       {"zmqhost",     1, 0, 'z'},
       {"zmqport",     1, 0, 'Z'},
       {"local",       1, 0, 'l'},
       {"receiver",    0, 0, 'r'},
       {"threaded",    0, 0, 'M'},
       {0,             0, 0,  0 }
    };

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned port  = 32410;
  unsigned zmqPort = 0;
  unsigned mask = 0;
  unsigned num_modules = 0;
  bool lUsage = false;
  bool isTriggered = false;
  bool isThreaded = false;
  bool configReceiver = true;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::Jungfrau, 0);
  char* uniqueid = (char *)NULL;
  char* zmqHost = (char *)NULL;
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
      case 'z':
        zmqHost = optarg;
        break;
      case 'Z':
        if (!CmdLineTools::parseUInt(optarg,zmqPort)) {
          printf("%s: option `-Z' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'l':
        if (!CmdLineTools::parseUInt(optarg,mask)) {
          printf("%s: option `-l' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'r':
        configReceiver = false;
        break;
      case 'M':
        isThreaded = true;
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

  if(num_modules > JungfrauConfigType::MaxModulesPerDetector) {
    printf("%s: number of modules exceeds the maximum per detector of %u\n",
           argv[0], JungfrauConfigType::MaxModulesPerDetector);
    return 1;
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

  if(zmqHost && zmqPort == 0) {
    printf("%s: zmqport is required with zmqhost\n", argv[0]);
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

  CfgClientNfs* cfg = new CfgClientNfs(detInfo);

  if (zmqHost)
    con = new Jungfrau::ZmqConnector(zmqHost, zmqPort);
  
  bool zmq_init_failed = false;
  for (unsigned i=0; i<num_modules; i++) {
    void* socket = NULL;
    // if using the zmq module builder force local mode for modules set as local
    if (con && !(mask & (1<<i))) {
      socket = con->bind(i, sHost[i], port);
      if (!socket) {
        printf("Failed to create Jungfrau zmq server for module %u\n", i);
        zmq_init_failed = true;
      }
    }
    modules[i] = new Jungfrau::Module(((detInfo.devId()&0xff)<<8) | (i&0xff), sSlsHost[i],
                                      sHost[i], port, sMac[i], sDetIp[i], configReceiver, socket);
  }
  if (zmq_init_failed) {
    printf("Aborting: Failed to create zmq receivers for Jungfrau modules\n");
    cleanup();
    return 1;
  }
  det = new Jungfrau::Detector(modules, isThreaded);
  if (!det->connected()) {
    printf("Aborting: Failed to connect to the Jungfrau detector, please check that it is present and powered!\n");
    cleanup();
    return 1;
  }

  // create task for connector
  if (con) {
    routine = new Jungfrau::ZmqConnectorRoutine(con, "JungfrauConnector");
    routine->enable();
  }

  Jungfrau::Server* srv = new Jungfrau::Server(detInfo);
  servers   .push_back(srv);
  Jungfrau::Manager* mgr = new Jungfrau::Manager(*det, *srv, *cfg);
  managers.push_back(mgr);

  StdSegWire settings(servers, uniqueid, MAX_MODULE_SIZE*num_modules + EVENT_SIZE_EXTRA, MAX_EVENT_DEPTH,
                      isTriggered, module, channel);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  if (seglevel->attach()) {
    task->mainLoop();
  }

  // cleanup the detector shmem from slsDetector and zeromq context
  cleanup();

  return 0;
}
