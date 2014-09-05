#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
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

namespace Pds {

  //
  //  This class creates the server when the streams are connected.
  //  Real implementations will have something like this.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(std::list<UsdUsb::Server*>& servers,
              unsigned module,
              unsigned channel,
              const char *aliasName) :
        _servers(servers),
        _module   (module),
        _channel  (channel)
    {
      for(std::list<UsdUsb::Server*>::iterator it=servers.begin(); it!=servers.end(); it++) {
	_sources.push_back((*it)->client()); 
        // only apply alias to the first server on the list
        if (aliasName && (it==servers.begin())) {
          SrcAlias tmpAlias((*it)->client(), aliasName);
          _aliases.push_back(tmpAlias);
        }
      }
    }
    virtual ~MySegWire() {}
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface) {
      for(std::list<UsdUsb::Server*>::iterator it=_servers.begin(); it!=_servers.end(); it++)
	wire.add_input(*it);
    }
    const std::list<Src>& sources() const { return _sources; }
    const std::list<SrcAlias>* pAliases() const
    {
      return (_aliases.size() > 0) ? &_aliases : NULL;
    }
    bool     is_triggered   () const { return true; }
    unsigned module         () const { return _module; }
    unsigned channel        () const { return _channel; }
    unsigned max_event_size () const { return 1024; }
    unsigned max_event_depth() const { return 256; }
  private:
    std::list<UsdUsb::Server*>& _servers;
    std::list<Src>              _sources;
    std::list<SrcAlias>         _aliases;
    unsigned                    _module;
    unsigned                    _channel;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class Seg : public EventCallback {
  public:
    Seg(Task*                     task,
        unsigned                  platform,
        SegWireSettings&          settings,
        Arp*                      arp,
        std::list<UsdUsb::Manager*>& managers) :
      _task      (task),
      _platform  (platform),
      _managers  (managers)
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
      printf("Seg connected to platform 0x%x\n",_platform);
      
      Stream* frmk = streams.stream(StreamParams::FrameWork);
      for(std::list<UsdUsb::Manager*>::iterator it=_managers.begin(); it!=_managers.end(); it++)
	(*it)->appliance().connect(frmk->inlet());
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
    Task*                        _task;
    unsigned                     _platform;
    std::list<UsdUsb::Manager*>& _managers;
  };
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
      if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
        printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
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

  std::list<UsdUsb::Server*>  servers;
  std::list<UsdUsb::Manager*> managers;

  UsdUsb::Server* srv = new UsdUsb::Server(detInfo);
  servers   .push_back(srv);
  UsdUsb::Manager* mgr = new UsdUsb::Manager(0, *srv, *new CfgClientNfs(detInfo));
  mgr->testTimeStep(tsc);
  managers.push_back(new UsdUsb::Manager(0, *srv, *new CfgClientNfs(detInfo)));

  Task* task = new Task(Task::MakeThisATask);
  MySegWire settings(servers, module, channel, uniqueid);
  Seg* seg = new Seg(task, platform, settings, 0, managers);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();

  return 0;
}
