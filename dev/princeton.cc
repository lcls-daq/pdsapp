#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <string>
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/epics/EpicsXtcSettings.hh"

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
#include "pds/princeton/PrincetonManager.hh"

using std::string;

namespace Pds 
{
static const char sPrincetonVersion[] = "1.21";
    
class SegWireSettingsPrinceton : public SegWireSettings 
{
public:
    SegWireSettingsPrinceton(const Src& src) { _sources.push_back(src); }
    virtual ~SegWireSettingsPrinceton() {}
    void connect (InletWire& wire, StreamParams::StreamType s, int interface) {}        
    const std::list<Src>& sources() const { return _sources; }
     
private:
    std::list<Src> _sources;
};
    
//
//    Implements the callbacks for attaching/dissolving.
//    Appliances can be added to the stream here.
//
class EventCallBackPrinceton : public EventCallback 
{
public:
    EventCallBackPrinceton(int iPlatform, CfgClientNfs& cfgService, int iCamera, bool bDelayMode, 
      bool bInitTest, string sConfigDb, int iSleepInt, int iDebugLevel) :
      _iPlatform(iPlatform), _cfg(cfgService), _iCamera(iCamera), 
      _bDelayMode(bDelayMode), _bInitTest(bInitTest), 
      _sConfigDb(sConfigDb), _iSleepInt(iSleepInt), _iDebugLevel(iDebugLevel),
      _bAttached(false), _princetonManager(NULL) 
    {
    }

    virtual ~EventCallBackPrinceton() 
    {
        reset();
    }
    
    bool IsAttached() { return _bAttached; }    
    
private:
    void reset()
    {
        delete _princetonManager;
        _princetonManager = NULL;
        
        _bAttached = false;
    }
    
    // Implements EventCallback
    virtual void attached(SetOfStreams& streams)        
    {        
        printf("Connected to iPlatform %d\n", _iPlatform);
             
        reset();        
        
        try
        {            
        _princetonManager = new PrincetonManager(_cfg, _iCamera, _bDelayMode, _bInitTest, _sConfigDb, _iSleepInt, _iDebugLevel);
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
      "[-l|--debug <level>] [-i|--id <id>] -p|--platform <platform id>\n" 
      "  Options:\n"
      "    -v|--version                 Show file version.\n"
      "    -h|--help                    Show usage.\n"
      "    -c|--camera   [0-9]          Select the princeton device. (Default: 0)\n"
      "    -i|--id       <id>           Set ID. Format: Detector/DetectorId/DeviceId. (Default: NoDetector/0/0)\n"
      "    -d|--delay                   Use delay mode.\n"
      "    -n|--init                    Run a testing capture to avoid the initial delay.\n"
      "    -g|--config   <db_path>      Intial princeton camera based on the config db at <db_path>\n"
      "    -s|--sleep    <sleep_ms>     Sleep inteval between multiple perinceton camera. (Default: 0 ms)\n"
      "    -l|--debug    <level>        Set debug level. (Default: 0)\n"
      "    -p|--platform <platform id>  [*required*] Set platform id.\n"
    );
}

static void showVersion()
{
    printf( "Version:  princeton  Ver %s\n", sPrincetonVersion );
}

static int    iSignalCaught   = 0;
static Task*  taskMainThread  = NULL;
void princetonSignalIntHandler( int iSignalNo )
{
  printf( "\nprincetonSignalIntHandler(): signal %d received. Stopping all activities\n", iSignalNo );
  iSignalCaught = 1;
  
  if (taskMainThread != NULL) 
    taskMainThread->destroy();     
}

int main(int argc, char** argv) 
{
    const char*   strOptions    = ":vhp:c:i:dnl:g:s:";
    const struct option loOptions[]   = 
    {
       {"ver",      0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"platform", 1, 0, 'p'},
       {"camera",   1, 0, 'c'},
       {"id",       1, 0, 'i'},
       {"delay",    0, 0, 'd'},
       {"init",     0, 0, 'n'},
       {"config",   1, 0, 'g'},
       {"sleep" ,   1, 0, 's'},
       {"debug" ,   1, 0, 'l'},
       {0,          0, 0,  0 }
    };    
    
    // parse the command line for our boot parameters
    int               iCamera       = 0;
    DetInfo::Detector detector      = DetInfo::NoDetector;
    int               iDetectorId   = 0;
    int               iDeviceId     = 0;
    bool              bDelayMode    = false;
    bool              bInitTest     = false;
    int               iDebugLevel   = 0;
    int               iPlatform     = -1;    
    string            sConfigDb;
    int               iSleepInt     = 0; // 0 ms
        
    int               iOptionIndex  = 0;
    while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt)
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;            
        case 'p':
            iPlatform = strtoul(optarg, NULL, 0);
            break;
        case 'c':
            iCamera = strtoul(optarg, NULL, 0);
            break;       
        case 'i':
            char* pNextToken;
            detector    = (DetInfo::Detector) strtoul(optarg, &pNextToken, 0); ++pNextToken;            
            if ( *pNextToken == 0 ) break;
            iDetectorId = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;            
            if ( *pNextToken == 0 ) break;
            iDeviceId   = strtoul(pNextToken, &pNextToken, 0);
            break;            
        case 'd':
            bDelayMode = true;
            break;            
        case 'n':
            bInitTest = true;
            break;            
        case 'l':
            iDebugLevel = strtoul(optarg, NULL, 0);
            break;       
        case 'g':
            sConfigDb = optarg;
            break;       
        case 's':
            iSleepInt = strtoul(optarg, NULL, 0);
            break;       
        case '?':               /* Terse output mode */
            printf( "princeton:main(): Unknown option: %c\n", optopt );
            break;
        case ':':               /* Terse output mode */
            printf( "princeton:main(): Missing argument for %c\n", optopt );
            break;
        default:            
        case 'h':               /* Print usage */
            showUsage();
            return 0;            
        }
    } 

    argc -= optind;
    argv += optind;

    if ( iPlatform == -1 ) 
    {   
        printf( "princeton:main(): Please specify platform in command line options\n" );
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

    printf("  Platform: %d  Camera: %d\n", iPlatform, iCamera);        
    
    const DetInfo detInfo( getpid(), detector, iDetectorId, DetInfo::Princeton, iDeviceId);    
    printf("  DetInfo: %s  ConfigDb: %s  Sleep: %d ms\n", DetInfo::name(detInfo), sConfigDb.c_str(), iSleepInt );    
    printf("  Delay Mode: %s  Init Test: %s  Debug Level: %d\n", (bDelayMode?"Yes":"No"), (bInitTest?"Yes":"No"),
      iDebugLevel);
      
    Task* task = new Task(Task::MakeThisATask);
    taskMainThread = task;
    
    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsPrinceton settings(detInfo);
    
    EventCallBackPrinceton  eventCallBackPrinceton(iPlatform, cfgService, iCamera, bDelayMode, bInitTest, sConfigDb, iSleepInt, iDebugLevel);
    SegmentLevel segmentLevel(iPlatform, settings, eventCallBackPrinceton, NULL);
    
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

// cpo: to avoid pvcam.so crappy dependency on firewire raw1394 shared library

extern "C" {

void raw1394_arm_register() {}
void raw1394_new_handle_on_port() {}
void raw1394_new_handle() {}
void raw1394_get_nodecount() {}
void raw1394_arm_get_buf() {}
void raw1394_arm_unregister() {}
void raw1394_destroy_handle() {}
void raw1394_read() {}
void raw1394_write() {}
void raw1394_get_port_info() {}
void raw1394_get_fd() {}
void raw1394_loop_iterate() {}

}
