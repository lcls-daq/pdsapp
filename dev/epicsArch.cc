#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <errno.h>
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
#include "pds/epicsArch/EpicsArchManager.hh"

using std::string;

namespace Pds 
{
static const char sEpicsArchVersion[] = "1.00";
    
class SegWireSettingsEpicsArch : public SegWireSettings 
{
public:
    SegWireSettingsEpicsArch(const Src& src) { _sources.push_back(src); }
    virtual ~SegWireSettingsEpicsArch() {}
    void connect (InletWire& wire, StreamParams::StreamType s, int interface) {}        
    const std::list<Src>& sources() const { return _sources; }
     
private:
    std::list<Src> _sources;
};
    
//
//    Implements the callbacks for attaching/dissolving.
//    Appliances can be added to the stream here.
//
class EvtCbEpicsArch : public EventCallback 
{
public:
    EvtCbEpicsArch(Task* task, int iPlatform, CfgClientNfs& cfgService, const string& sFnConfig, 
      float fMinTriggerInterval, int iDebugLevel) :
      _task(task), _iPlatform(iPlatform), _cfg(cfgService), _sFnConfig(sFnConfig), 
      _fMinTriggerInterval(fMinTriggerInterval), _iDebugLevel(iDebugLevel),
      _bAttached(false), _epicsArchManager(NULL), _pool(sizeof(Transition),1)
    {
        //// !! For debug test only
        //_epicsArchManager = new EpicsArchManager(_cfg, _sFnConfig);        
    }

    virtual ~EvtCbEpicsArch()
    {
        reset();
    }
    
    bool IsAttached() { return _bAttached; }    
    
private:
    void reset()
    {
        delete _epicsArchManager;
        _epicsArchManager = NULL;
        
        _bAttached = false;
    }
    
    // Implements EventCallback
    virtual void attached(SetOfStreams& streams)        
    {        
        printf("EvtCbEpicsArch connected to iPlatform 0x%x\n", 
             _iPlatform);

        Stream* frmk = streams.stream(StreamParams::FrameWork);
     
        reset();        
        _epicsArchManager = new EpicsArchManager(_cfg, _sFnConfig, _fMinTriggerInterval, _iDebugLevel);
        _epicsArchManager->appliance().connect(frmk->inlet());
        _bAttached = true;

        streams.wire(StreamParams::FrameWork)->post(*new (&_pool)Transition(TransitionId::Reset,0));
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
    string              _sFnConfig;
    float               _fMinTriggerInterval;
    int                 _iDebugLevel;
    bool                _bAttached;
    EpicsArchManager*   _epicsArchManager;    
    GenericPool         _pool;
}; // class EvtCbEpicsArch


} // namespace Pds 


using namespace Pds;


static void showUsage()
{
    printf( "Usage:  epicsArch  [-v|--version] [-h|--help] "
      "[-d|--debug <debug level>] [-n|--unit <unit #> -p|--platform <platform>  -f <config filename>\n" 
      "  Options:\n"
      "    -v|--version       Show file version\n"
      "    -h|--help          Show usage\n"
      "    -d|--debug         Set debug level\n"
      "    -n|--unit          Set unit number [Default: 0]\n"
      "    -f|--file          [*required*] Set configuration filename\n"
      "    -p|--platform      [*required*] Set platform id\n"
      " ================================================================================\n"
      "  Config File Format:\n"
      "    - Each line of the file can contain one PV name\n"
      "    - Use \'#\' at the beginning of the line to comment out whole line\n"
      "    - Use \'#\' in the middle of the line to comment out the remaining characters\n"
      "    - Use '*' at the beginning of the line to define an alias for the immediately following PV(s)\n"
      "    - Use \'<\' to include file(s)\n"
      "  \n"
      "  Example:\n"
      "    %%cat epicsArch.txt\n"
      "    < PvList0.txt, PvList1.txt # Include Two Files\n"      
      "    iocTest:aiExample          # PV Name\n"
      "    # This is a comment line\n"
      "    iocTest:calcExample1\n"      
      "    * electron beam energy     # Alias for BEND:DMP1:400:BDES\n"
      "    BEND:DMP1:400:BDES\n"
    );
}

static void showVersion()
{
    printf( "Version:  epicArch  Ver %s\n", sEpicsArchVersion );
}

int main(int argc, char** argv) 
{
    const char*         strOptions  = ":vhi:d:p:f:n:";
    const struct option loOptions[] = 
    {
       {"version",  0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"debug",    1, 0, 'd'},
       {"interval", 1, 0, 'i'},
       {"platform", 1, 0, 'p'},
       {"file",     1, 0, 'f'},
       {"unit",     1, 0, 'n'},
       {0,          0, 0,  0  }
    };    
    
    int     iPlatform           = -1;
    float   fMinTriggerInterval = 0.0f;
    string  sFnConfig;
    int     iDebugLevel         = 0;
    int     iUnit               = 0;

    int     iOptionIndex        = 0;
    char *  endptr;
    while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt) 
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;            
        case 'd':
            iDebugLevel = strtoul(optarg, NULL, 0);
            break;            
        case 'i':
            printf("interval recording of data no longer supported -\n"
                   "\tnow recorded with every event.\n");
            break;            
        case 'p':
            errno = 0;
            iPlatform = strtoul(optarg, &endptr, 0);
            if ((optarg == endptr) || errno) {
              printf( "epicsArch:main(): Invalid platform number\n");
            }
            break;
        case 'f':
            sFnConfig = optarg;
            break;
        case 'n':
            errno = 0;
            iUnit = strtoul(optarg, &endptr, 0);
            if ((optarg == endptr) || errno) {
              printf( "epicsArch:main(): Invalid unit number\n");
            }
            break;
        case '?':               /* Terse output mode */
            printf( "epicsArch:main(): Unknown option: %c\n", optopt );
            break;
        case ':':               /* Terse output mode */
            printf( "epicsArch:main(): Missing argument for %c\n", optopt );
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
        printf( "epicsArch:main(): Please specify platform in command line\n" );
        showUsage();
        return 1;
    }
    if ( sFnConfig.empty() ) 
    {   
        printf( "epicsArch:main(): Please specify config filename in command line\n" );
        showUsage();
        return 2;
    }

    const DetInfo detInfo( getpid(), Pds::DetInfo::EpicsArch, 0, DetInfo::NoDevice, iUnit);    

    Task* task = new Task(Task::MakeThisATask);

    // keep this: it's the "hook" into the configuration database
    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsEpicsArch settings(detInfo);
    
    EvtCbEpicsArch evtCBEpicsArch(task, iPlatform, cfgService, sFnConfig, fMinTriggerInterval, iDebugLevel);
    SegmentLevel seglevel(iPlatform, settings, evtCBEpicsArch, NULL);
    
    seglevel.attach();    
    //    if ( evtCBEpicsArch.IsAttached() )    
        task->mainLoop(); // Enter the event processing loop, and never returns (unless the program terminates)
        
    return 0;
}
