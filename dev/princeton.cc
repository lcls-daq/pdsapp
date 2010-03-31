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
#include "pds/princeton/SegmentEventLevel.hh"

using std::string;

namespace Pds 
{
static const char sPrincetonVersion[] = "0.90";
    
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
    EventCallBackPrinceton(int iPlatform, CfgClientNfs& cfgService, bool bDelayMode, int iDebugLevel) :
      _iPlatform(iPlatform), _cfg(cfgService), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel),
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
        printf("Connected to iPlatform %d, delay mode %d, debug level %d\n", 
             _iPlatform, (_bDelayMode?1:0), _iDebugLevel);
             
        reset();        
        
        try
        {            
        _princetonManager = new PrincetonManager(_cfg, _bDelayMode, _iDebugLevel);
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
    bool                _bDelayMode;
    int                 _iDebugLevel;
    bool                _bAttached;
    PrincetonManager*   _princetonManager;    
}; // class EventCallBackPrinceton


} // namespace Pds 


using namespace Pds;


static void showUsage()
{
    printf( "Usage:  princeton  [-v|--version] [-h|--help] [-d|--delay <0|1>]"
      "[-l|--debug <level>] -e|--evrip <evr ip>  -p|--platform <platform id>\n" 
      "  Options:\n"
      "    -v|--version                 Show file version\n"
      "    -h|--help                    Show usage\n"
      "    -d|--delay    <0|1>          Use delay mode or not\n"
      "    -l|--debug    <level>        Set debug level\n"
      "    -e|--evrip    <evr ip>       [*required*] Set EVR ip address\n"
      "    -p|--platform <platform id>  [*required*] Set platform id\n"
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
    const char*   strOptions    = ":vhdl:e:p:";
    struct option loOptions[]   = 
    {
       {"ver",      0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"delay",    0, 0, 'd'},       
       {"debug",    1, 0, 'l'},
       {"evrip",    1, 0, 'e'},       
       {"platform", 1, 0, 'p'},
       {0,          0, 0,  0  }
    };    
    
    // parse the command line for our boot parameters
    bool    bDelayMode    = false;
    int     iDebugLevel   = 0;
    string  sEvrIp;
    int     iPlatform     = -1;
    
    int     iOptionIndex  = 0;
    while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt)
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;            
        case 'd':
            bDelayMode = true;
            break;            
        case 'l':
            iDebugLevel = strtoul(optarg, NULL, 0);
            break;            
        case 'e':
            sEvrIp = optarg;
            break;
        case 'p':
            iPlatform = strtoul(optarg, NULL, 0);
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

    if ( sEvrIp == "" ) 
    {   
        printf( "princeton:main(): Please specify EVR IP in command line options\n" );
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
      
    const DetInfo detInfo( getpid(), DetInfo::NoDetector, 0, DetInfo::Princeton, 0);    

    Task* task = new Task(Task::MakeThisATask);
    taskMainThread = task;

    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsPrinceton settings(detInfo);
    
    EventCallBackPrinceton  eventCallBackPrinceton(iPlatform, cfgService, bDelayMode, iDebugLevel);
    SegmentEventLevel       segEventlevel(sEvrIp.c_str(), iPlatform, settings, eventCallBackPrinceton, NULL);
    //SegmentLevel segEventlevel(iPlatform, settings, EventCallBackPrinceton, NULL); // !! for debug
    
    segEventlevel.attach();    
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
 
