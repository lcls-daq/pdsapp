#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string>
#include "pdsdata/xtc/DetInfo.hh"

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
    SegWireSettingsEpicsArch() {}
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
class EvtCBEpicsArch : public EventCallback 
{
public:
    EvtCBEpicsArch(Task* task, unsigned int uPlatform, CfgClientNfs& cfgService, const string& sFnConfig, float fMinTriggerInterval ) :
      _task(task), _uPlatform(uPlatform), _cfg(cfgService), _sFnConfig(sFnConfig), _fMinTriggerInterval(fMinTriggerInterval),
      _bAttached(false), _epicsArchmgr(NULL)  
    {
        //// !! For debug test only
        //_epicsArchmgr = new EpicsArchManager(_cfg, _sFnConfig);        
    }

    virtual ~EvtCBEpicsArch()
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
        printf("EvtCBEpicsArch connected to uPlatform 0x%x\n", 
             _uPlatform);

        Stream* frmk = streams.stream(StreamParams::FrameWork);
        // you'll need a Manager. the Manager
        // is notified when it's time to stop/start, or
        // send the data out.    This is "higher level" idea.
     
        reset();        
        _epicsArchmgr = new EpicsArchManager(_cfg, _sFnConfig, _fMinTriggerInterval);
        _epicsArchmgr->appliance().connect(frmk->inlet());
        _bAttached = true;
    }
    
    virtual void failed(Reason reason)    
    {
        static const char* reasonname[] = { "platform unavailable", 
                                        "crates unavailable", 
                                        "fcpm unavailable" };
        printf("Seg: unable to allocate crates on uPlatform 0x%x : %s\n", 
             _uPlatform, reasonname[reason]);
             
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
    unsigned int        _uPlatform;
    CfgClientNfs&       _cfg;
    string              _sFnConfig;
    float               _fMinTriggerInterval;
    bool                _bAttached;
    EpicsArchManager*   _epicsArchmgr;    
}; // class EvtCBEpicsArch


} // namespace Pds 


using namespace Pds;


static void showUsage()
{
    printf( "Usage:  epicsArch  [-v|--version] [-h|--help] [-d|--detctor <detid>] [-p|--platform <platform>] "
      "[-a|--arp <arp process id>] [-i|--interval <min trigger interval>] <config filename>\n" 
      "  Options:\n"
      "    -v|--version       Show file version\n"
      "    -h|--help          Show Usage\n"
      "    -d|--detector      Set detctor id\n"
      "    -p|--platform      Set platform id\n"
      "    -a|--arp           Set arp process id\n"
      "    -i|--interval      Set minimum trigger interval, in seconds (float value)\n"
      "  <config filename>    Configuration File\n" );
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
       {"detector", 1, 0, 'd'},
       {"platform", 1, 0, 'p'},
       {"arp",      1, 0, 'a'},
       {"interval", 1, 0, 'i'},
       {0,          0, 0,  0  }
    };    
    
    // parse the command line for our boot parameters
    unsigned int detid = -1UL;
    unsigned int uPlatform = -1UL;
    Arp* arp = NULL;
    float fMinTriggerInterval = 1.0f;

    
    while ( int opt = getopt_long(argc, argv, ":vhd:p:a:i:", loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt) 
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;            
        case 'd':
            detid    = strtoul(optarg, NULL, 0);
            break;
        case 'p':
            uPlatform = strtoul(optarg, NULL, 0);
            break;
        case 'a':
            arp = new Arp(optarg);
            break;
        case 'i':
            fMinTriggerInterval = (float) strtod(optarg, NULL);
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


    if ( argc < 1 ) 
    {   
        printf( "epicsArch:main(): Command Line Syntax Incorrect\n" );
        showUsage();
        return 1;        
    }    
    else if ( uPlatform == -1UL || detid == -1UL ) 
    {   
        printf( "epicsArch:main(): Please specify platform and detector ID in command line\n\n" );
        showUsage();
        return 2;
    }
    
    string sFnConfig(argv[0]);
    
    // launch the SegmentLevel
    if (arp) 
    {
        if (arp->error()) 
        {
            printf( "epicsArch:main(): failed to create odfArp : %s\n", strerror(arp->error()));
            delete arp;
            return 3;
        }
    }

    // need to put in new numbers in DetInfo.hh for the epicsArch
    Node node(Level::Source,uPlatform);
    DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detid, 0, DetInfo::Acqiris, 0);

    Task* task = new Task(Task::MakeThisATask);

    // Local scope: task and arp remain as valid pointers during the life time of this local scope
    {
        // keep this: it's the "hook" into the configuration database
        CfgClientNfs cfgService = CfgClientNfs(detInfo);
        SegWireSettingsEpicsArch settings;
        
        EvtCBEpicsArch evtCBEpicsArch(task, uPlatform, cfgService, sFnConfig, fMinTriggerInterval);
        SegmentLevel seglevel(uPlatform, settings, evtCBEpicsArch, arp);
        
        seglevel.attach();    
        if ( evtCBEpicsArch.IsAttached() )    
            task->mainLoop();          
    }
    
    task->destroy(); 
    delete arp;
    
    return 0;
}
