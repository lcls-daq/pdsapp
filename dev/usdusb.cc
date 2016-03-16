#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/usdusb/Manager.hh"
#include "pds/usdusb/Server.hh"
#include "pds/config/CfgClientNfs.hh"
#include "usdusb4/include/libusdusb4.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "libusb.h"

#include <list>

extern int optind;

static int reset_usb()
{
  int n = 0;

  libusb_context* pctx;

  libusb_init(&pctx);

  const int vid = 0x09c9;
  const int pid = 0x0044;

  libusb_device_handle* phdl = libusb_open_device_with_vid_pid(pctx, vid, pid);
  if (phdl) {
    libusb_reset_device(phdl);
    libusb_close(phdl);
    n = 1;
  }

  libusb_exit(pctx);

  return n;
}

static void close_usb(int isig)
{
  printf("close_usb %d\n",isig);
  //  USB4_Shutdown();
  const char* nsem = "Usb4-0000";
  printf("Unlinking semaphore %s\n",nsem);
  if (sem_unlink(nsem))
    perror("Error unlinking usb4 semaphore");
  exit(0);
}

using namespace Pds;

static void usdUsbUsage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "    -i <detinfo>                int/int/int/int or string/int/string/int\n"
         "                                  (e.g. XppEndStation/0/USDUSB/1 or 22/0/26/1)\n"
         "    -p <platform>,<mod>,<chan>  platform number, EVR module, EVR channel\n"
         "    -z                          zeroes encoder counts\n"
         "    -u <alias>                  set device alias\n"
         "    -t                          disable testing time step check\n"
         "    -h                          print this message and exit\n", p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  bool lzero = false;
  bool lUsage = false;
  bool tsc = true;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::USDUSB, 0);
  char* uniqueid = (char *)NULL;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:p:u:zth")) != EOF ) {
    switch(c) {
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,detInfo)) {
        printf("%s: option `-i' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'p':
      if (CmdLineTools::parseUInt(optarg,platform,module,channel) != 3) {
        printf("%s: option `-p' parsing error\n", argv[0]);
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
    case 'z':
      lzero = true;
      break;
    case 't':
      tsc = false;
      break;
    case 'h': // help
      usdUsbUsage(argv[0]);
      return 0;
    case '?':
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
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usdUsbUsage(argv[0]);
    return 1;
  }

   printf("UsdUsb is %sabling testing time step check\n", tsc ? "en" : "dis");

  //
  //  There must be a way to detect multiple instruments, but I don't know it yet
  //
  reset_usb();

  short deviceCount = 0;
  printf("Initializing device\n");
  int result = USB4_Initialize(&deviceCount);
  if (result != USB4_SUCCESS) {
    printf("Failed to initialize USB4 driver (%d)\n",result);
    close_usb(0);
    return 1;
  }

  //
  //  Need to shutdown the USB driver properly
  //
  struct sigaction int_action;

  int_action.sa_handler = close_usb;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;

  if (sigaction(SIGINT, &int_action, 0) > 0)
    printf("Couldn't set up SIGINT handler\n");
  if (sigaction(SIGKILL, &int_action, 0) > 0)
    printf("Couldn't set up SIGKILL handler\n");
  if (sigaction(SIGSEGV, &int_action, 0) > 0)
    printf("Couldn't set up SIGSEGV handler\n");
  if (sigaction(SIGABRT, &int_action, 0) > 0)
    printf("Couldn't set up SIGABRT handler\n");
  if (sigaction(SIGTERM, &int_action, 0) > 0)
    printf("Couldn't set up SIGTERM handler\n");

  printf("Found %d devices\n", deviceCount);

  if (lzero) {
    for(unsigned i=0; i<UsdUsbConfigType::NCHANNELS; i++) {
      if ((result = USB4_SetPresetValue(deviceCount, i, 0)) != USB4_SUCCESS)
	printf("Failed to set preset value for channel %d : %d\n",i, result);
      if ((result = USB4_ResetCount(deviceCount, i)) != USB4_SUCCESS)
	printf("Failed to set preset value for channel %d : %d\n",i, result);
    }
    close_usb(0);
    return 1;
  }

  std::list<EbServer*>        servers;
  std::list<UsdUsb::Manager*> managers;

  UsdUsb::Server* srv = new UsdUsb::Server(detInfo);
  servers   .push_back(srv);
  UsdUsb::Manager* mgr = new UsdUsb::Manager(0, *srv, *new CfgClientNfs(detInfo));
  mgr->testTimeStep(tsc);
  managers.push_back(new UsdUsb::Manager(0, *srv, *new CfgClientNfs(detInfo)));

  StdSegWire settings(servers, uniqueid, 1024, 256, true, module, channel);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();

  return 0;
}
