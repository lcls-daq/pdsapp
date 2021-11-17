#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/vimba/Manager.hh"
#include "pds/vimba/Server.hh"
#include "pds/vimba/Driver.hh"
#include "pds/vimba/Errors.hh"
#include "pds/vimba/FrameBuffer.hh"
#include "pds/vimba/ConfigCache.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <list>

static const unsigned AT_MAX_MSG_LEN = 256;
static const unsigned EVENT_SIZE_EXTRA = 0x10000;
static const unsigned MAX_EVENT_SIZE = 2*5328*4608 + EVENT_SIZE_EXTRA;
static const unsigned MAX_EVENT_DEPTH = 64;
static const char* GENICAM_ENV = "GENICAM_GENTL64_PATH";

static Pds::Vimba::Camera* cam = NULL;

using namespace Pds;

static void cleanup(bool complete=true)
{
  if (cam) {
    if (complete) {
      delete cam;
      cam = NULL;
    } else {
      cam->close();
    }
  }
  VmbShutdown();
}

static void shutdown(int isig)
{
  cleanup(false);
  exit(isig);
}

static void vimbaUsage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "    -i|--id       <detinfo>                 int/int/int/int or string/int/string/int\n"
         "                                            (e.g. XppEndStation/0/Alvium/1 or 22/0/44/1)\n"
         "    -p|--platform <platform>,<mod>,<chan>   platform number, EVR module, EVR channel\n"
         "    -u|--uniqueid <alias>                   set device alias\n"
         "    -c|--camera   [0-9]                     select the Vimba SDK index. (default: 0)\n"
         "    -s|--serialid <serialid>                find camera via serialid otherwise index is used\n"
         "    -b|--buffers  <buffers>                 the number of frame buffers to provide to the Vimba SDK (default: 64)\n"
         "    -l|--limit    <link limit>              limit the link speed to this value (default: 450000000 Bytes/s)\n"
         "    -w|--sloweb   <0/1/2>                   set slow readout mode (default: 0)\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:c:s:b:l:w:";
  const struct option loOptions[]   =
    {
       {"help",        0, 0, 'h'},
       {"platform",    1, 0, 'p'},
       {"id",          1, 0, 'i'},
       {"uniqueid",    1, 0, 'u'},
       {"camera",      1, 0, 'c'},
       {"serialid",    1, 0, 's'},
       {"buffers",     1, 0, 'b'},
       {"limit",       1, 0, 'l'},
       {"sloweb",      1, 0, 'w'},
       {0,             0, 0,  0 }
    };

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned num_buffers = 64;
  unsigned link_limit = 450000000;
  int camera_index = 0;
  int slowReadout = 0;
  bool lUsage = false;
  bool isTriggered = false;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::Alvium, 0);
  char* uniqueid = (char *)NULL;
  const char* serial_id = NULL;
  VmbError_t err = VmbErrorSuccess;
  VmbCameraInfo_t info;
  VmbVersionInfo_t version;
  
  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        vimbaUsage(argv[0]);
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
        if (!CmdLineTools::parseInt(optarg,camera_index)) {
          printf("%s: option `-c' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 's':
        serial_id = optarg;
        break;
      case 'b':
        if (!CmdLineTools::parseUInt(optarg,num_buffers)) {
          printf("%s: option `-b' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'l':
        if (!Pds::CmdLineTools::parseUInt(optarg,link_limit)) {
          printf("%s: option `-l' parsing error\n", argv[0]);
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
        lUsage = true;
        break;
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
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
    vimbaUsage(argv[0]);
    return 1;
  }

  if(slowReadout==0) {
    printf("Setting normal readout mode for vimba process.\n");
  } else {
    printf("Setting slow readout mode for vimba process!\n");
  }

  // if the genicam path is not found set the default one
  setenv(GENICAM_ENV, Vimba::Camera::getGeniCamPath().c_str(), 0);
  // show the genicam path
  printf("Using %s=\"%s\"\n", GENICAM_ENV, getenv(GENICAM_ENV));

  // initialize the vimba sdk
  if ((err = VmbStartup()) != VmbErrorSuccess) {
    printf("Failed to initialize Vimba SDK!: %s\n", Vimba::ErrorCodes::desc(err));
    return 1;
  }

  // get vimba version info
  if (Vimba::Camera::getVersionInfo(&version)) {
    printf("Using Vimba SDK version %u.%u.%u\n", version.major, version.minor, version.patch);
  } else {
    printf("Failed to query Vimba SDK version! - exitting...\n");
    VmbShutdown();
    return 1;
  }

  // Retrieve camera info
  if (Vimba::Camera::getCameraInfo(&info, camera_index, serial_id)) {
    // list info on the camera
    printf("Found camera:\n"
           "  Name       - %s\n"
           "  Model      - %s\n"
           "  ID         - %s\n"
           "  Serial Num - %s\n"
           "  Interface  - %s\n",
           info.cameraName,
           info.modelName,
           info.cameraIdString,
           info.serialString,
           info.interfaceIdString);
    // Initialize the camera object
    cam = new Vimba::Camera(&info);
    if (cam->isOpen()) {
      if (!cam->setDeviceLinkThroughputLimit(link_limit)) {
        printf("Failed to configure device link limit to %u Bytes/s\n", link_limit);
        cleanup();
        return 1;
      }
    } else {
      printf("Failed to open camera (%s)!\n", info.cameraIdString);
      cleanup();
      return 1;
    }
  } else {
    if (serial_id) {
      printf("Failed to find a camera with serial number %s!\n", serial_id);
    } else {
      printf("Failed to find a camera with Vimba SDK index %d!\n", camera_index);
    }
    cleanup();
    return 1;
  }

  Vimba::ConfigCache* cfg = Vimba::ConfigCache::create(detInfo, *cam);
  if (!cfg) {
    printf("Unable to create ConfigCache for %s!\n", DetInfo::name(detInfo));
    cleanup();
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

  std::list<EbServer*>      servers;
  std::list<Vimba::Manager*> managers;

  Vimba::Server* srv = new Vimba::Server(detInfo);
  servers.push_back(srv);

  Vimba::FrameBuffer* buf = new Vimba::FrameBuffer(num_buffers, cam, srv);

  Vimba::Manager* mgr = new Vimba::Manager(*buf, *srv, *cfg);
  managers.push_back(mgr);

  StdSegWire settings(servers, uniqueid, MAX_EVENT_SIZE, MAX_EVENT_DEPTH, isTriggered, module, channel);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0, slowReadout);
  if (seglevel->attach()) {
    task->mainLoop();
  }

  // cleanup camera and shutdown vimba sdk
  cleanup();

  return 0;
}
