#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
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
    EvtCbPrinceton(Task* task, int iPlatform, CfgClientNfs& cfgService, string sFnOtuput, int iDebugLevel) :
      _task(task), _iPlatform(iPlatform), _cfg(cfgService), _sFnOtuput(sFnOtuput), _iDebugLevel(iDebugLevel),
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
        printf("EvtCbPrinceton connected to iPlatform 0x%x\n", 
             _iPlatform);

        Stream* frmk = streams.stream(StreamParams::FrameWork);
     
        reset();        
        _princetonManager = new PrincetonManager(_cfg, _sFnOtuput, _iDebugLevel);
        _princetonManager->appliance().connect(frmk->inlet());
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
    Task*               _task;
    int                 _iPlatform;
    CfgClientNfs&       _cfg;
    string              _sFnOtuput;
    int                 _iDebugLevel;
    bool                _bAttached;
    PrincetonManager*   _princetonManager;    
}; // class EvtCbPrinceton


} // namespace Pds 


using namespace Pds;


static void showUsage()
{
    printf( "Usage:  princeton  [-v|--version] [-h|--help] [-f <output filename>]"
      "[-d|--debug <debug level>] -p|--platform <platform>\n" 
      "  Options:\n"
      "    -v|--version       Show file version\n"
      "    -h|--help          Show usage\n"
      "    -f|--file          Set output filename - If not set, data is sent to event level nodes\n"
      "    -d|--debug         Set debug level\n"
      "    -p|--platform      [*required*] Set platform id\n"
    );
}

static void showVersion()
{
    printf( "Version:  epicArch  Ver %s\n", sPrincetonVersion );
}

int main(int argc, char** argv) 
{
    int iOptionIndex = 0;
    struct option loOptions[] = 
    {
       {"ver",      0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"file",     1, 0, 'f'},       
       {"debug",    1, 0, 'd'},
       {"platform", 1, 0, 'p'},
       {0,          0, 0,  0  }
    };    
    
    // parse the command line for our boot parameters
    int     iPlatform   = -1;
    int     iDebugLevel = 0;
    string  sFnOtuput;
    
    while ( int opt = getopt_long(argc, argv, ":vhf:d:p:", loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt) 
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;            
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
    
    try
    {   
      
    const DetInfo detInfo( getpid(), Pds::DetInfo::NoDetector, 0, DetInfo::Princeton, 0);    

    Task* task = new Task(Task::MakeThisATask);

    // keep this: it's the "hook" into the configuration database
    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsPrinceton settings(detInfo);
    
    EvtCbPrinceton evtCBPrinceton(task, iPlatform, cfgService, sFnOtuput, iDebugLevel);
    SegmentLevel seglevel(iPlatform, settings, evtCBPrinceton, NULL);
    
    seglevel.attach();    
    if ( evtCBPrinceton.IsAttached() )    
        task->mainLoop(); // Enter the event processing loop, and never returns (unless the program terminates)
        
    }
    catch ( PrincetonManagerException& eManager )
    {
      printf( "main(): PrincetonManager exception %s\n", eManager.what() );
    }
    catch (...)
    {
      printf( "main(): Unknown exception\n" );
    }
    
    return 0;
}
 
