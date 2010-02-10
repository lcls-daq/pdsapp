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
class EvtCbPrinceton : public EventCallback 
{
public:
    EvtCbPrinceton(int iPlatform, CfgClientNfs& cfgService, bool bMakeUpEvent, string sFnOtuput, int iDebugLevel) :
      _iPlatform(iPlatform), _cfg(cfgService), _bMakeUpEvent(bMakeUpEvent), _sFnOtuput(sFnOtuput), _iDebugLevel(iDebugLevel),
      _bAttached(false), _princetonManager(NULL)  
    {
        //// !! For debug test only
        //_princetonManager = new PrincetonManager(_cfg, _sFnConfig);        
    }

    virtual ~EvtCbPrinceton()
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
        printf("Connected to iPlatform %d, debug level %d\n", 
             _iPlatform, _iDebugLevel);
             
        reset();        
        
        try
        {            
        _princetonManager = new PrincetonManager(_cfg, _bMakeUpEvent, _sFnOtuput, _iDebugLevel);
        }
        catch ( PrincetonManagerException& eManager )
        {
          printf( "EvtCbPrinceton::attached(): PrincetonManager init failed, error message = \n  %s\n", eManager.what() );
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
    bool                _bMakeUpEvent;
    string              _sFnOtuput;
    int                 _iDebugLevel;
    bool                _bAttached;
    PrincetonManager*   _princetonManager;    
}; // class EvtCbPrinceton


} // namespace Pds 


using namespace Pds;


static void showUsage()
{
    printf( "Usage:  princeton  [-v|--version] [-h|--help] [-m|--makeup] [-f|--file <output filename>]"
      "[-d|--debug <debug level>] -p|--platform <platform>\n" 
      "  Options:\n"
      "    -v|--version       Show file version\n"
      "    -h|--help          Show usage\n"
      "    -m|--makeup        Use makeup event mode\n"
      "    -f|--file          Set output filename - If not set, default to use stream mode\n"
      "    -d|--debug         Set debug level\n"
      "    -p|--platform      [*required*] Set platform id\n"
    );
}

static void showVersion()
{
    printf( "Version:  epicArch  Ver %s\n", sPrincetonVersion );
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
    int iOptionIndex = 0;
    struct option loOptions[] = 
    {
       {"ver",      0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"makeup",   0, 0, 'm'},       
       {"file",     1, 0, 'f'},       
       {"debug",    1, 0, 'd'},
       {"platform", 1, 0, 'p'},
       {0,          0, 0,  0  }
    };    
    
    // parse the command line for our boot parameters
    int     iPlatform     = -1;
    bool    bMakeUpEvent  = false;
    string  sFnOtuput;
    int     iDebugLevel   = 0;
    
    while ( int opt = getopt_long(argc, argv, ":vhmf:d:p:", loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt) 
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;            
        case 'm':
            bMakeUpEvent = true;
            break;            
        case 'f':
            sFnOtuput = optarg;
            break;            
        case 'd':
            iDebugLevel = strtoul(optarg, NULL, 0);
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
        printf( "princeton:main(): Please specify platform in command line\n" );
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
      
    const DetInfo detInfo( getpid(), Pds::DetInfo::NoDetector, 0, DetInfo::Princeton, 0);    

    Task* task = new Task(Task::MakeThisATask);
    taskMainThread = task;

    // keep this: it's the "hook" into the configuration database
    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsPrinceton settings(detInfo);
    
    EvtCbPrinceton evtCBPrinceton(iPlatform, cfgService, bMakeUpEvent, sFnOtuput, iDebugLevel);
    SegmentLevel seglevel(iPlatform, settings, evtCBPrinceton, NULL);
    
    seglevel.attach();    
    if ( evtCBPrinceton.IsAttached() )    
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
 
