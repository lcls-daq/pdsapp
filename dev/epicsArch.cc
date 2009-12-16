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
#include "pds/epicsArch/EpicsArchManager.hh"

using std::string;

namespace Pds 
{
static const char sEpicsArchVersion[] = "0.90";
    
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
      _bAttached(false), _epicsArchmgr(NULL)  
    {
        //// !! For debug test only
        //_epicsArchmgr = new EpicsArchManager(_cfg, _sFnConfig);        
    }

    virtual ~EvtCbEpicsArch()
    {
        reset();
    }
    
    bool IsAttached() { return _bAttached; }    
    
private:
    void reset()
    {
        delete _epicsArchmgr;
        _epicsArchmgr = NULL;
        
        _bAttached = false;
    }
    
    // Implements EventCallback
    virtual void attached(SetOfStreams& streams)        
    {        
        printf("EvtCbEpicsArch connected to iPlatform 0x%x\n", 
             _iPlatform);

        Stream* frmk = streams.stream(StreamParams::FrameWork);
     
        reset();        
        _epicsArchmgr = new EpicsArchManager(_cfg, _sFnConfig, _fMinTriggerInterval, _iDebugLevel);
        _epicsArchmgr->appliance().connect(frmk->inlet());
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
    string              _sFnConfig;
    float               _fMinTriggerInterval;
    int                 _iDebugLevel;
    bool                _bAttached;
    EpicsArchManager*   _epicsArchmgr;    
}; // class EvtCbEpicsArch


} // namespace Pds 


using namespace Pds;


static void showUsage()
{
    printf( "Usage:  epicsArch  [-v|--version] [-h|--help] [-i|--interval <min trigger interval>] "
      "[-d|--debug <debug level>] -p|--platform <platform>  -f <config filename>\n" 
      "  Options:\n"
      "    -v|--version       Show file version\n"
      "    -h|--help          Show Usage\n"
      "    -d|--debug         Set debug level\n"
      "    -i|--interval      Set minimum trigger interval, in seconds (float number) [Default: 1.0]\n"
      "    -f|--file          [*required*] Set configuration filename\n"
      "    -p|--platform      [*required*] Set platform id\n"
    );
}

static void showVersion()
{
    printf( "Version:  epicArch  Ver %s\n", sEpicsArchVersion );
}

int main(int argc, char** argv) 
{
    int iOptionIndex = 0;
    struct option loOptions[] = 
    {
       {"ver",      0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"debug",    1, 0, 'd'},
       {"interval", 1, 0, 'i'},
       {"platform", 1, 0, 'p'},
       {"file",     1, 0, 'f'},
       {0,          0, 0,  0  }
    };    
    
    // parse the command line for our boot parameters
    int iPlatform = -1;
    float fMinTriggerInterval = 1.0f;
    string sFnConfig;
    int iDebugLevel = 0;
    
    while ( int opt = getopt_long(argc, argv, ":vhi:d:p:f:", loOptions, &iOptionIndex ) )
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
            fMinTriggerInterval = (float) strtod(optarg, NULL);
            break;            
        case 'p':
            iPlatform = strtoul(optarg, NULL, 0);
            break;
        case 'f':
            sFnConfig = optarg;
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
        
    const DetInfo& detInfo = EpicsXtcSettings::detInfo;

    Task* task = new Task(Task::MakeThisATask);

    // keep this: it's the "hook" into the configuration database
    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsEpicsArch settings(detInfo);
    
    EvtCbEpicsArch evtCBEpicsArch(task, iPlatform, cfgService, sFnConfig, fMinTriggerInterval, iDebugLevel);
    SegmentLevel seglevel(iPlatform, settings, evtCBEpicsArch, NULL);
    
    seglevel.attach();    
    if ( evtCBEpicsArch.IsAttached() )    
        task->mainLoop(); // Enter the event processing loop, and never returns (unless the program terminates)
        
    return 0;
}
