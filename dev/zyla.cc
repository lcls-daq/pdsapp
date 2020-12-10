#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/zyla/Manager.hh"
#include "pds/zyla/Server.hh"
#include "pds/zyla/Driver.hh"
#include "pds/config/CfgClientNfs.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <list>

static const unsigned AT_MAX_MSG_LEN = 256;
static const unsigned EVENT_SIZE_EXTRA = 0x10000;
static const unsigned MAX_EVENT_SIZE = 2*2560*2160 + EVENT_SIZE_EXTRA;
static const unsigned MAX_EVENT_DEPTH = 64;

static const AT_WC* AT3_SIMCAM_ID = L"SIMCAM CMOS";

using namespace Pds;

static AT_H Handle = AT_HANDLE_UNINITIALISED;
static const int MAX_CONN_RETRY = 10;

static void close_camera(int isig)
{
  if (Handle!=AT_HANDLE_UNINITIALISED) {
    AT_Close(Handle);
  }
  AT_FinaliseUtilityLibrary();
  AT_FinaliseLibrary();
  exit(0);
}

static void zylaUsage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "    -i|--id       <detinfo>                 int/int/int/int or string/int/string/int\n"
         "                                            (e.g. XppEndStation/0/Zyla/1 or 22/0/44/1)\n"
         "    -p|--platform <platform>,<mod>,<chan>   platform number, EVR module, EVR channel\n"
         "    -u|--uniqueid <alias>                   set device alias\n"
         "    -c|--camera   [0-9]                     select the camera device index (default: 0)\n"
         "    -b|--buffers  <buffers>                 the number of frame buffers to provide to the Andor SDK (default: 5)\n"
         "    -W|--wait                               wait for camera temperature to stabilize before running\n"
         "    -w|--sloweb   <0/1/2>                   set slow readout mode (default: 0)\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:c:b:Ww:";
  const struct option loOptions[]   =
    {
       {"help",        0, 0, 'h'},
       {"platform",    1, 0, 'p'},
       {"id",          1, 0, 'i'},
       {"uniqueid",    1, 0, 'u'},
       {"camera",      1, 0, 'c'},
       {"buffers",     1, 0, 'b'},
       {"wait",        0, 0, 'W'},
       {"sloweb",      1, 0, 'w'},
       {0,             0, 0,  0 }
    };

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned num_buffers = 5;
  int camera = 0;
  int slowReadout = 0;
  bool lUsage = false;
  bool isTriggered = false;
  bool waitCooling = false;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::Zyla, 0);
  char* uniqueid = (char *)NULL;
  
  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        zylaUsage(argv[0]);
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
      case 'b':
        if (!CmdLineTools::parseUInt(optarg,num_buffers)) {
          printf("%s: option `-b' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'W':
        waitCooling = true;
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
    zylaUsage(argv[0]);
    return 1;
  }

  if(slowReadout==0) {
    printf("Setting normal readout mode for zyla process.\n");
  } else {
    printf("Setting slow readout mode for zyla process!\n");
  }

  // Initialize Andor SDK
  if (AT_InitialiseLibrary() != AT_SUCCESS) {
    printf("Failed to initialize Andor SDK! - exitting...\n");
    return 1;
  }

  if (AT_InitialiseUtilityLibrary() != AT_SUCCESS) {
    printf("Failed to initialize Andor SDK Utilities! - exitting...\n");
    return 1;
  }

  // Get the number of devices
  AT_64 dev_count;
  if (AT_GetInt(AT_HANDLE_SYSTEM, L"Device Count", &dev_count) != AT_SUCCESS) {
    printf("Failed to retrieve device count from SDK! - exitting...\n");
    return 1;
  }

  if (camera > (dev_count-1)) {
    printf("Requested camera index (%d) is out of range of the %lld available cameras!\n", camera, dev_count);
    return 1;
  }

  //AT_H Handle;
  AT_Open(camera, &Handle);

  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = close_camera;
  sigActionSettings.sa_flags   = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGINT\n");
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGTERM\n");

  Zyla::Driver* drv = new Zyla::Driver(Handle, num_buffers);
  bool cam_timeout = false;
  int retry_count = 0;
  printf("Waiting to camera to initialize ...");
  while(!drv->is_present()) {
    if (retry_count > MAX_CONN_RETRY) break;
    printf(".");
    sleep(1);
    retry_count++;
  }
  if (cam_timeout) {
    printf(" timeout!\n");
    printf("Camera is not present!\n");
    AT_Close(Handle);
    AT_FinaliseUtilityLibrary();
    AT_FinaliseLibrary();
    return 1;
  } else {
    printf(" done!\n");
  }

  // Print out basic camera info
  bool isSimCam = false;
  AT_WC wc_buffer[AT_MAX_MSG_LEN];
  printf("Camera information:\n");
  drv->get_model(wc_buffer, AT_MAX_MSG_LEN);
  printf(" Camera model: %ls\n", wc_buffer);
  // Check if the camera is a simcam
  if (wcscmp(AT3_SIMCAM_ID, wc_buffer) == 0) {
    isSimCam = true;
  }
  drv->get_name(wc_buffer, AT_MAX_MSG_LEN);
  printf(" Camera name: %ls\n", wc_buffer);
  drv->get_family(wc_buffer, AT_MAX_MSG_LEN);
  printf(" Camera family: %ls\n", wc_buffer);
  drv->get_serial(wc_buffer, AT_MAX_MSG_LEN);
  printf(" Camera serial number: %ls\n", wc_buffer);
  drv->get_firmware(wc_buffer, AT_MAX_MSG_LEN);
  printf(" Camera firmware revision: %ls\n", wc_buffer);
  drv->get_interface_type(wc_buffer, AT_MAX_MSG_LEN);
  printf(" Camera interface type: %ls\n", wc_buffer);
  drv->get_sdk_version(wc_buffer, AT_MAX_MSG_LEN);
  printf(" Andor sdk version: %ls\n", wc_buffer);
  printf(" Camera sensor width in pixels: %lld\n", drv->sensor_width());
  printf(" Camera sensor height in pixels: %lld\n", drv->sensor_height());
  printf(" Camera pixel width (um): %g\n", drv->pixel_width());
  printf(" Camera pixel height (um): %g\n", drv->pixel_height());

  if (isSimCam) {
    // If all we find is a simcam this means the Andor SDK didn't find any real cameras connected to the machine
    printf("Aborting: Failed to find any Zyla/Neo cameras, please check that it is present and powered!\n");
  } else {
    std::list<EbServer*>      servers;
    std::list<Zyla::Manager*> managers;

    CfgClientNfs* cfg = new CfgClientNfs(detInfo);
  
    Zyla::Server* srv = new Zyla::Server(detInfo);
    servers.push_back(srv);
    Zyla::Manager* mgr = new Zyla::Manager(*drv, *srv, *cfg, waitCooling);
    managers.push_back(mgr);

    StdSegWire settings(servers, uniqueid, MAX_EVENT_SIZE, MAX_EVENT_DEPTH, isTriggered, module, channel);

    Task* task = new Task(Task::MakeThisATask);
    EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
    SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0, slowReadout);
    if (seglevel->attach()) {
      task->mainLoop();
    }
  }

  // Clean up camera
  drv->close();
  AT_FinaliseUtilityLibrary();
  AT_FinaliseLibrary();

  return isSimCam?1:0;
}
