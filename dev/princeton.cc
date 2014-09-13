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
#include "pds/service/CmdLineTools.hh"
#include "pds/princeton/PrincetonManager.hh"

using std::string;

namespace Pds
{
static const char sPrincetonVersion[] = "1.21";

class SegWireSettingsPrinceton : public SegWireSettings
{
public:
    SegWireSettingsPrinceton(const Src& src,
                             bool               bTriggered,
                             unsigned           uModule,
                             unsigned           uChannel,
                             string sAliasName) :
      _bTriggered (bTriggered),
      _uModule    (uModule),
      _uChannel   (uChannel)
    {
      _sources.push_back(src);
      if (sAliasName.length())
      {
        SrcAlias tmpAlias(src, sAliasName.c_str());
        _aliases.push_back(tmpAlias);
      }
    }
    virtual ~SegWireSettingsPrinceton() {}
    void connect (InletWire& wire, StreamParams::StreamType s, int interface) {}
    const std::list<Src>& sources() const { return _sources; }
    const std::list<SrcAlias>* pAliases() const
    {
      return (_aliases.size() > 0) ? &_aliases : NULL;
    }
    bool     is_triggered() const { return _bTriggered; }
    unsigned module      () const { return _uModule; }
    unsigned channel     () const { return _uChannel; }
private:
    std::list<Src> _sources;
    std::list<SrcAlias> _aliases;
    bool           _bTriggered;
    unsigned       _uModule;
    unsigned       _uChannel;
};

//
//    Implements the callbacks for attaching/dissolving.
//    Appliances can be added to the stream here.
//
class EventCallBackPrinceton : public EventCallback
{
public:
    EventCallBackPrinceton(int iPlatform, CfgClientNfs& cfgService, int iCamera, bool bDelayMode,
      bool bInitTest, string sConfigDb, int iSleepInt, int iCustW, int iCustH, int iDebugLevel) :
      _iPlatform(iPlatform), _cfg(cfgService), _iCamera(iCamera),
      _bDelayMode(bDelayMode), _bInitTest(bInitTest),
      _sConfigDb(sConfigDb), _iSleepInt(iSleepInt), _iCustW(iCustW), _iCustH(iCustH),
      _iDebugLevel(iDebugLevel),
      _bAttached(false), _princetonManager(NULL)
    {
    }

    virtual ~EventCallBackPrinceton()
    {
        reset();
    }

    bool IsAttached() { return _bAttached; }

    void reset()
    {
        delete _princetonManager;
        _princetonManager = NULL;

        _bAttached = false;
    }
private:
    // Implements EventCallback
    virtual void attached(SetOfStreams& streams)
    {
        printf("Connected to iPlatform %d\n", _iPlatform);

        reset();

        try
        {
        _princetonManager = new PrincetonManager(_cfg, _iCamera, _bDelayMode, _bInitTest, _sConfigDb, _iSleepInt, _iCustW, _iCustH, _iDebugLevel);
        _princetonManager->initServer();
        }
        catch ( PrincetonManagerException& eManager )
        {
          printf( "EventCallBackPrinceton::attached(): PrincetonManager init failed, error message = \n  %s\n", eManager.what() );
          return;
        }

        Stream* streamFramework = streams.stream(StreamParams::FrameWork);
        _princetonManager->appliance().connect(streamFramework->inlet());
        _bAttached = true;
    }

    virtual void failed(Reason reason)
    {
        static const char* reasonname[] = { "platform unavailable",
                                        "crates unavailable",
                                        "fcpm unavailable" };
        printf("Seg: unable to allocate crates on iPlatform 0x%x : %s\n",
             _iPlatform, reasonname[reason]);

        reset();
    }

    virtual void dissolved(const Node& who)
    {
        const unsigned userlen = 12;
        char username[userlen];
        Node::user_name(who.uid(),username,userlen);

        const unsigned iplen = 64;
        char ipname[iplen];
        Node::ip_name(who.ip(),ipname, iplen);

        printf("Seg: platform 0x%x dissolved by user %s, pid %d, on node %s",
             who.platform(), username, who.pid(), ipname);

        reset();
    }

private:
    int                 _iPlatform;
    CfgClientNfs&       _cfg;
    int                 _iCamera;
    bool                _bDelayMode;
    bool                _bInitTest;
    string              _sConfigDb;
    int                 _iSleepInt;
    int                 _iCustW;
    int                 _iCustH;
    int                 _iDebugLevel;
    bool                _bAttached;
    PrincetonManager*   _princetonManager;
}; // class EventCallBackPrinceton


} // namespace Pds


using namespace Pds;


static void showUsage()
{
    printf( "Usage:  princeton  [-v|--version] [-h|--help] [-c|--camera <0-9> ] "
      "[-i|--id <id>] [-d|--delay] [-n|--init] [-g|--config <db_path>] [-s|--sleep <ms>] "
      "[-l|--debug <level>] [-u|--uniqueid <alias>] [--custw <width>] [--custh <heighth>] "
      "-p|--platform <platform>[,<mod>,<chan>]\n"
      "  Options:\n"
      "    -v|--version                             Show file version.\n"
      "    -h|--help                                Show usage.\n"
      "    -c|--camera   [0-9]                      Select the princeton device. (Default: 0)\n"
      "    -i|--id       <id>                       Set ID. Format: Detector/DetectorId/DeviceId. (Default: NoDetector/0/0)\n"
      "    -u|--uniqueid <alias>                    Set device alias.\n"
      "    -d|--delay                               Use delay mode.\n"
      "    -n|--init                                Run a testing capture to avoid the initial delay.\n"
      "    -g|--config   <db_path>                  Initialize princeton camera based on the config db at <db_path>\n"
      "    -s|--sleep    <sleep_ms>                 Sleep interval between multiple princeton cameras. (Default: 0 ms)\n"
      "    -l|--debug    <level>                    Set debug level. (Default: 0)\n"
      "    -p|--platform <platform>[,<mod>,<chan>]  Set platform id [*required*], EVR module, EVR channel\n"
      "    --custw       <width>                    Set custom width\n"
      "    --custh       <width>                    Set custom height\n"
    );
}

static void showVersion()
{
    printf( "Version:  princeton  Ver %s\n", sPrincetonVersion );
}

static int    iSignalCaught   = 0;
static Task*  taskMainThread  = NULL;
static EventCallBackPrinceton* pEventCallbackPrinceton = NULL;
void princetonSignalIntHandler( int iSignalNo )
{
  printf( "\nprincetonSignalIntHandler(): signal %d received. Stopping all activities\n", iSignalNo );
  iSignalCaught = 1;

  if (pEventCallbackPrinceton)
    pEventCallbackPrinceton->reset();

  exit(1);

  //if (taskMainThread != NULL)
  //  taskMainThread->destroy();
}

int main(int argc, char** argv)
{
    const char*   strOptions    = ":vhp:c:i:u:dnl:g:s:";
    const struct option loOptions[]   =
    {
       {"ver",      0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"platform", 1, 0, 'p'},
       {"camera",   1, 0, 'c'},
       {"id",       1, 0, 'i'},
       {"uniqueid", 1, 0, 'u'},
       {"delay",    0, 0, 'd'},
       {"init",     0, 0, 'n'},
       {"config",   1, 0, 'g'},
       {"sleep" ,   1, 0, 's'},
       {"debug" ,   1, 0, 'l'},
       {"custw" ,   1, 0, 101},
       {"custh" ,   1, 0, 102},
       {0,          0, 0,  0 }
    };

    // parse the command line for our boot parameters
    int               iCamera       = 0;
    DetInfo::Detector detector      = DetInfo::NoDetector;
    int               iDetectorId   = 0;
    int               iDeviceId     = 0;
    string            sUniqueId;
    bool              bDelayMode    = true; // always use delay mode
    bool              bInitTest     = false;
    int               iDebugLevel   = 0;
    int               iPlatform     = -1;
    unsigned          uModule       = 0;
    unsigned          uChannel      = 0;
    string            sConfigDb;
    int               iSleepInt     = 0; // 0 ms
    int               iCustW        = -1;
    int               iCustH        = -1;
    int               iOptionIndex  = 0;
    bool              bShowUsage    = false;
    bool              bTriggered    = false;
    unsigned          uu1, uu2, uu3;

    while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;

        switch(opt)
        {
        case 'v':               /* Print usage */
            showVersion();
            return 0;
        case 'p':
            switch (CmdLineTools::parseUInt(optarg,uu1,uModule,uChannel))
            {
            case 1:
              bTriggered = false;
              break;
            case 3:
              bTriggered = true;
              break;
            default:
              printf("princeton:main(): option `-p' parsing error\n");
              bShowUsage = true;
              break;
            }
            iPlatform = (int) uu1;
            break;
        case 'c':
            if (!CmdLineTools::parseInt(optarg, iCamera))
            {
              printf("princeton:main(): option `-c' parsing error\n");
              bShowUsage = true;
            }
            break;
        case 'i':
            if (CmdLineTools::parseUInt(optarg,uu1,uu2,uu3,0,'/') != 3)
            {
              printf("princeton:main(): option `-i' parsing error\n");
              bShowUsage = true;
            }
            else
            {
              detector = (DetInfo::Detector) uu1;
              iDetectorId = (int) uu2;
              iDeviceId = (int) uu3;
            }
            break;
        case 'u':
            sUniqueId = optarg;
            break;
        case 'd':
            bDelayMode = true;
            break;
        case 'n':
            bInitTest = true;
            break;
        case 'l':
            if (!CmdLineTools::parseInt(optarg,iDebugLevel))
            {
              printf("princeton:main(): option `-l' parsing error\n");
              bShowUsage = true;
            }
            break;
        case 'g':
            sConfigDb = optarg;
            break;
        case 's':
            if (!CmdLineTools::parseInt(optarg,iSleepInt))
            {
              printf("princeton:main(): option `-s' parsing error\n");
              bShowUsage = true;
            }
            break;
        case 101:
            if (!CmdLineTools::parseInt(optarg,iCustW))
            {
              printf("princeton:main(): option `--custw' parsing error\n");
              bShowUsage = true;
            }
            break;
        case 102:
            if (!CmdLineTools::parseInt(optarg,iCustH))
            {
              printf("princeton:main(): option `--custh' parsing error\n");
              bShowUsage = true;
            }
            break;
        case '?':               /* Terse output mode */
            if (optopt)
              printf( "princeton:main(): Unknown option: %c\n", optopt );
            else
              printf( "princeton:main(): Unknown option: %s\n", argv[optind-1] );
            bShowUsage = true;
            break;
        case ':':               /* Terse output mode */
            printf( "princeton:main(): Missing argument for %c\n", optopt );
            bShowUsage = true;
            break;
        default:
        case 'h':               /* Print usage */
            bShowUsage = true;
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if ( iPlatform == -1 )
    {
        printf( "princeton:main(): Please specify platform in command line options\n" );
        bShowUsage = true;
    }
    if (argc != 0)
    {
      printf( "princeton:main(): Some command line options are not processed.\n" );
      bShowUsage = true;
    }

    if (bShowUsage)
    {
      showUsage();
      return 1;
    }

    /*
     * Register singal handler
     */
    struct sigaction sigActionSettings;
    sigemptyset(&sigActionSettings.sa_mask);
    sigActionSettings.sa_handler = princetonSignalIntHandler;
    sigActionSettings.sa_flags   = SA_RESTART;

    if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
      printf( "main(): Cannot register signal handler for SIGINT\n" );
    if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
      printf( "main(): Cannot register signal handler for SIGTERM\n" );

    try
    {
    printf("Settings:\n");

    if (bTriggered)
      printf("  Platform: %d  EVR module: %u  EVR channel: %u  Camera: %d\n", iPlatform, uModule, uChannel, iCamera);
    else
      printf("  Platform: %d  Camera: %d\n", iPlatform, iCamera);

    const DetInfo detInfo( getpid(), detector, iDetectorId, DetInfo::Princeton, iDeviceId);
    printf("  DetInfo: %s  ConfigDb: %s  Sleep: %d ms\n", DetInfo::name(detInfo), sConfigDb.c_str(), iSleepInt );
    printf("  Delay Mode: %s  Init Test: %s  Debug Level: %d\n", (bDelayMode?"Yes":"No"), (bInitTest?"Yes":"No"),
      iDebugLevel);
    if (iCustW != -1 || iCustH != -1)
    printf("  Custom W: %d  H: %d\n", iCustW, iCustH);
    if (sUniqueId.length())
      printf("  Alias: %s\n", sUniqueId.c_str());

    Task* task = new Task(Task::MakeThisATask);
    taskMainThread = task;

    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsPrinceton settings(detInfo, bTriggered, uModule, uChannel, sUniqueId);

    EventCallBackPrinceton  eventCallBackPrinceton(iPlatform, cfgService, iCamera, bDelayMode, bInitTest, sConfigDb, iSleepInt, iCustW, iCustH, iDebugLevel);

    /* slow readout = 0, becuase we do the data polling by ourselves, not by event builder */
    SegmentLevel segmentLevel(iPlatform, settings, eventCallBackPrinceton, NULL, 0);

    pEventCallbackPrinceton = &eventCallBackPrinceton;

    segmentLevel.attach();
    if ( eventCallBackPrinceton.IsAttached() )
        task->mainLoop(); // Enter the event processing loop, and never returns (unless the program terminates)
    }
    catch (...)
    {
      if ( iSignalCaught != 0 )
        exit(0);
      else
        printf( "main(): Unknown exception\n" );
    }

    return 0;
}
