#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <string>
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/epics.ddl.h"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/Task.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/oceanoptics/OceanOpticsManager.hh"
#include "pds/oceanoptics/OceanOpticsServer.hh"

using std::string;

namespace Pds
{
  static const char sOceanOpticsVersion[] = "0.9";

  class SegWireSettingsOceanOptics:public SegWireSettings
  {
  public:
    SegWireSettingsOceanOptics(const Src & src, OceanOpticsServer* pServer, string sAliasName) :
      _pServer(pServer)
    {
      _sources.push_back(src);
      if (sAliasName.length())
      {
        SrcAlias tmpAlias(src, sAliasName.c_str());
        _aliases.push_back(tmpAlias);
      }
    }

    virtual ~SegWireSettingsOceanOptics()
    {
    }
    void connect(InletWire & wire, StreamParams::StreamType s, int interface)
    {
      printf("Adding input of server, fd %d\n", _pServer->fd() );
      wire.add_input( _pServer );
    }
    const std::list < Src > &sources() const
    {
      return _sources;
    }

    const std::list<SrcAlias>* pAliases() const
    {
      return (_aliases.size() > 0) ? &_aliases : NULL;
    }

    unsigned max_event_size () const { return 128*1024; }

    unsigned max_event_depth() const { return 128; }

  private:
    std::list < Src >   _sources;
    OceanOpticsServer*  _pServer;
    std::list<SrcAlias> _aliases;
  };

//
//    Implements the callbacks for attaching/dissolving.
//    Appliances can be added to the stream here.
//
  class EventCallBackOceanOptics:public EventCallback
  {
  public:
    EventCallBackOceanOptics(OceanOpticsServer* pServer,
      int iPlatform, CfgClientNfs & cfgService,
      int iDevice, int iDebugLevel):
      _pServer(pServer), _pManager(NULL), _iPlatform(iPlatform), _cfg(cfgService), _iDevice(iDevice),
      _iDebugLevel(iDebugLevel), _bAttached(false)
    {
    }

    virtual ~EventCallBackOceanOptics()
    {
      reset();
    }

    bool IsAttached()
    {
      return _bAttached;
    }

  private:
    void reset()
    {
      delete _pManager;
      _pManager = NULL;

      _bAttached = false;
    }

    // Implements EventCallback
    virtual void attached(SetOfStreams & streams)
    {
      printf("Connected to iPlatform %d\n", _iPlatform);

      reset();

      _pManager = new OceanOpticsManager(_cfg, _pServer, _iDevice, _iDebugLevel);

      Stream *streamFramework = streams.stream(StreamParams::FrameWork);
      _pManager->appliance().connect(streamFramework->inlet());
      _bAttached = true;
    }

    virtual void failed(Reason reason)
    {
      static const char *reasonname[] =
      { "platform unavailable",
        "crates unavailable",
        "fcpm unavailable"
      };
      printf("Seg: unable to allocate crates on iPlatform 0x%x : %s\n",
       _iPlatform, reasonname[reason]);

      reset();
    }

    virtual void dissolved(const Node & who)
    {
      const unsigned userlen = 12;
      char username[userlen];
      Node::user_name(who.uid(), username, userlen);

      const unsigned iplen = 64;
      char ipname[iplen];
      Node::ip_name(who.ip(), ipname, iplen);

      printf("Seg: platform 0x%x dissolved by user %s, pid %d, on node %s",
       who.platform(), username, who.pid(), ipname);

      reset();
    }

  private:
    OceanOpticsServer*  _pServer;
    OceanOpticsManager* _pManager;
    int                 _iPlatform;
    CfgClientNfs &      _cfg;
    int                 _iDevice;
    int                 _iDebugLevel;
    bool                _bAttached;
  };        // class EventCallBackOceanOptics


}       // namespace Pds


using namespace Pds;


static void showUsage()
{
  printf
    ("Usage:  oceanoptics  [-v|--version] [-h|--help] [-d|--device <0-9> ]\n"
     "                     [-i|--id <id>] [-l|--debug <level>] [-u|--uniqueid <alias>]\n"
     "                     -p|--platform <platform id>\n"
     "  Options:\n" "    -v|--version                 Show file version.\n"
     "    -h|--help                    Show usage.\n"
     "    -d|--device   [0-9]          Select the oceanOptics device. (Default: 0)\n"
     "    -i|--id       <id>           Set ID. Format: Detector/DetectorId/DeviceId. (Default: NoDetector/0/0)\n"
     "    -u|--uniqueid <alias>        Set device alias.\n"
     "    -l|--debug    <level>        Set debug level. (Default: 0)\n"
     "    -p|--platform <platform id>  [*required*] Set platform id.\n");
}

static void showVersion()
{
  printf("Version:  oceanOptics  Ver %s\n", sOceanOpticsVersion);
}

static int iSignalCaught = 0;
static Task *taskMainThread = NULL;
void oceanOpticsSignalIntHandler(int iSignalNo)
{
  printf
    ("\noceanOpticsSignalIntHandler(): signal %d received. Stopping all activities\n",
     iSignalNo);
  iSignalCaught = 1;

  if (taskMainThread != NULL)
    taskMainThread->destroy();
}

int main(int argc, char **argv)
{
  const char *strOptions = ":vhp:d:i:u:l:";
  const struct option loOptions[] = {
    {"ver"      , 0, 0, 'v'},
    {"help"     , 0, 0, 'h'},
    {"platform" , 1, 0, 'p'},
    {"device"   , 1, 0, 'd'},
    {"id"       , 1, 0, 'i'},
    {"uniqueid" , 1, 0, 'u'},
    {"debug"    , 1, 0, 'l'},
    {0, 0, 0, 0}
  };

  // parse the command line for our boot parameters
  int               iDevice     = 0;
  DetInfo::Detector detector    = DetInfo::NoDetector;
  int               iDetectorId = 0;
  int               iDeviceId   = 0;
  string            sUniqueId;
  int               iDebugLevel = 0;
  int               iPlatform   = -1;

  int iOptionIndex = 0;
  while (int opt =
   getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex))
  {
    if (opt == -1)
      break;

    switch (opt)
    {
    case 'v':     /* Print usage */
      showVersion();
      return 0;
    case 'p':
      iPlatform = strtoul(optarg, NULL, 0);
      break;
    case 'd':
      iDevice = strtoul(optarg, NULL, 0);
      break;
    case 'i':
      char *pNextToken;
      detector = (DetInfo::Detector) strtoul(optarg, &pNextToken, 0);
      ++pNextToken;
      if (*pNextToken == 0)
        break;

      iDetectorId = strtoul(pNextToken, &pNextToken, 0);
      ++pNextToken;
      if (*pNextToken == 0)
        break;

      iDeviceId = strtoul(pNextToken, &pNextToken, 0);
      break;
    case 'u':
      sUniqueId = optarg;
      break;
    case 'l':
      iDebugLevel = strtoul(optarg, NULL, 0);
      break;
    case '?':               /* Terse output mode */
      if (optopt)
        printf( "oceanOptics:main(): Unknown option: %c\n", optopt );
      else
        printf( "oceanOptics:main(): Unknown option: %s\n", argv[optind-1] );
      break;
    case ':':     /* Terse output mode */
      printf("oceanOptics:main(): Missing argument for %c\n", optopt);
      break;
    default:
    case 'h':     /* Print usage */
      showUsage();
      return 0;
    }
  }

  argc -= optind;
  argv += optind;

  if (iPlatform == -1)
  {
    printf
      ("oceanOptics:main(): Please specify platform in command line options\n");
    showUsage();
    return 1;
  }

  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = oceanOpticsSignalIntHandler;
  sigActionSettings.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0)
    printf("main(): Cannot register signal handler for SIGINT\n");
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0)
    printf("main(): Cannot register signal handler for SIGTERM\n");

  try
  {
    printf("Settings:\n");

    printf("  Platform: %d  Device: %d\n", iPlatform, iDevice);

    const DetInfo detInfo(getpid(), detector, iDetectorId,
        DetInfo::OceanOptics, iDeviceId);
    printf("  DetInfo: %s  Debug Level: %d\n", DetInfo::name(detInfo), iDebugLevel);
    if (sUniqueId.length())
      printf("  Alias: %s\n", sUniqueId.c_str());

    Task *task = new Task(Task::MakeThisATask);
    taskMainThread = task;

    CfgClientNfs cfgService = CfgClientNfs(detInfo);

    // Note: pServer will be deleted inside ~SegementLevel
    OceanOpticsServer* pServer = new OceanOpticsServer(cfgService.src(), iDevice, iDebugLevel);

    EventCallBackOceanOptics eventCallBackOceanOptics(pServer, iPlatform, cfgService, iDevice, iDebugLevel);

    SegWireSettingsOceanOptics settings(detInfo, pServer, sUniqueId);
    SegmentLevel segmentLevel(iPlatform, settings, eventCallBackOceanOptics, NULL);

    segmentLevel.attach();
    if (eventCallBackOceanOptics.IsAttached())
      task->mainLoop();   // Enter the event processing loop, and never returns (unless the program terminates)

    // Note: ~SegmentLevel() will delete the server internally, so we don't call delete pServer here
  }
  catch ( OceanOpticsServerException& eServer )
  {
    printf
      ("main(): OceanOpticsServer init failed, error message = \n  %s\n",
       eServer.what());
  }
  catch(...)
  {
    if (iSignalCaught != 0)
      exit(0);
    else
      printf("main(): Unknown exception\n");
  }

  return 0;
}
