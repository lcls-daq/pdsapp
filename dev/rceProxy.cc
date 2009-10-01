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
#include "pds/rceProxy/RceProxyManager.hh"

using std::string;

namespace Pds 
{
static const char sRceProxyVersion[] = "0.90";
    
class SegWireSettingsRceProxy : public SegWireSettings 
{
public:
    SegWireSettingsRceProxy(const Src& src) { _sources.push_back(src); }
    virtual ~SegWireSettingsRceProxy() {}
    void connect (InletWire& wire, StreamParams::StreamType s, int interface) {}        
    const std::list<Src>& sources() const { return _sources; }
     
private:
    std::list<Src> _sources;
};
    
//
//    Implements the callbacks for attaching/dissolving.
//    Appliances can be added to the stream here.
//
class EvtCBRceProxy : public EventCallback 
{
public:
    EvtCBRceProxy(Task* task, int iPlatform, CfgClientNfs& cfgService, const string& sRceIp, 
      int iNumLinks, int iPayloadSizePerLink, int iDebugLevel) :
      _task(task), _iPlatform(iPlatform), _cfg(cfgService), _sRceIp(sRceIp), 
      _iNumLinks(iNumLinks), _iPayloadSizePerLink(iPayloadSizePerLink), _iDebugLevel(iDebugLevel),
      _bAttached(false), _rceProxymgr(NULL), _pSelfNode(NULL)
    {
    }

    virtual ~EvtCBRceProxy()
    {
        reset();
    }
    
    bool IsAttached()                       { return _bAttached; }
    void setSelfNode(const Node& selfNode)  { _pSelfNode = &selfNode; }
    
private:
    void reset()
    {
        delete _rceProxymgr;
        _rceProxymgr = NULL;
        
        _bAttached = false;
    }
    
    // Implements EventCallback
    virtual void attached(SetOfStreams& streams)        
    {        
        printf("EvtCBRceProxy connected to iPlatform 0x%x\n", 
             _iPlatform);

        Stream* frmk = streams.stream(StreamParams::FrameWork);
        // you'll need a Manager. the Manager
        // is notified when it's time to stop/start, or
        // send the data out.    This is "higher level" idea.
     
        reset();        
        _rceProxymgr = new RceProxyManager(_cfg, _sRceIp, _iNumLinks, _iPayloadSizePerLink, *_pSelfNode, _iDebugLevel);
        _rceProxymgr->appliance().connect(frmk->inlet());
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
    string              _sRceIp;
    int                 _iNumLinks;
    int                 _iPayloadSizePerLink;
    int                 _iDebugLevel;
    bool                _bAttached;
    RceProxyManager*    _rceProxymgr;
    const Node*         _pSelfNode;
}; // class EvtCBRceProxy


} // namespace Pds 


using namespace Pds;

static void showUsage()
{
    printf( "Usage: rceProxy [-x|--version] [-h|--help] [-d|--debug <debug level>] "
      "[-t|--detector <detector type>] [-i|--detid <detector id>] [-v|--device <device type>] [-e|--devid <device id>] "
      "[-n|--numlinks <number of links>] [-a|--payloadsize <payload size per link>]"
      "-p|--platform <platform id>  -r|--rceip <rce ip> \n" 
      "  Options:\n"
      "    -x|--version       Show file version\n"
      "    -h|--help          Show Usage\n"
      "    -d|--debug         Set debug level\n"
      "    -t|--detector      Set detector type         [Default: 8(Camp)]\n"
      "    -i|--detid         Set detector id           [Default: 0]\n"
      "    -v|--device        Set device type           [Default: 5(pnCCD)]\n"
      "    -e|--devid         Set device id             [Default: 0]\n"
      "    -n|--numlinks      Set number of links       [Default: 2]\n"
      "    -a|--payloadsize   Set payload size per link [Default: 524304]\n"
      "    -p|--platform      [*required*] Set platform id \n"
      "    -r|--rceip         [*required*] Set host IP address of RCE \n"
    );
}

static void showVersion()
{
    printf( "Version:  rceProxy  Ver %s\n", sRceProxyVersion );
}

int main(int argc, char** argv) 
{
    int iOptionIndex = 0;
    struct option loOptions[] = 
    {
       {"ver",          0, 0, 'x'},
       {"help",         0, 0, 'h'},
       {"debug",        1, 0, 'd'},
       {"detector",     1, 0, 't'},
       {"detId",        1, 0, 'i'},
       {"device",       1, 0, 'v'},
       {"devId",        1, 0, 'e'},
       {"numlinks",     1, 0, 'n'},
       {"payloadsize",  1, 0, 'a'},
       {"platform",     1, 0, 'p'},
       {"rceIp",        1, 0, 'r'},
       {0,              0, 0,  0 },
    };    
    
    // parse the command line for our boot parameters
    int                 iDebugLevel = 0;
    DetInfo::Detector   detector = DetInfo::Camp;
    int                 iDetectorId = 0;
    DetInfo::Device     device  = DetInfo::pnCCD;
    int                 iDeviceId = 0;
    int                 iNumLinks = 2;
    int                 iPayloadSizePerLink = 524304;
    int                 iPlatform = -1;
    string              sRceIp;
    
    while ( int opt = getopt_long(argc, argv, ":xhd:t:i:v:e:n:a:p:r:", loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt) 
        {            
        case 's':
            showVersion();
            return 0;            
        case 'h':               
        default:            
            showUsage();
            return 0;            
        case 'd':
            iDebugLevel = strtoul(optarg, NULL, 0);
            break;            
        case 't':
            detector = (DetInfo::Detector) strtoul(optarg, NULL, 0);
            break;            
        case 'i':
            iDetectorId = strtoul(optarg, NULL, 0);
            break;            
        case 'v':
            device = (DetInfo::Device) strtoul(optarg, NULL, 0);
            break;            
        case 'e':
            iDeviceId = strtoul(optarg, NULL, 0);
            break;            
        case 'n':
            iNumLinks = strtoul(optarg, NULL, 0);
            break;            
        case 'a':
            iPayloadSizePerLink = strtoul(optarg, NULL, 0);
            break;            
        case 'p':
            iPlatform = strtol(optarg, NULL, 0);
            break;
        case 'r':
            sRceIp = optarg;
            break;            
        case '?':               /* Terse output mode */
            printf( "rceProxy:main(): Unknown option: %c\n", optopt );
            break;
        case ':':               /* Terse output mode */
            printf( "rceProxy:main(): Missing argument for %c\n", optopt );
            break;            
        }
    }

    argc -= optind;
    argv += optind;

    /* Check if command line option(s) is valid */    
    if ( iPlatform == -1 ) 
    {   
        printf( "rceProxy:main(): Please specify platform in command line\n" );
        showUsage();
        return 1;
    }
    if ( sRceIp.empty() ) 
    {   
        printf( "rceProxy:main(): Please specify RCE IP in command line\n" );
        showUsage();
        return 2;
    }    
    
    // print summary of command line options and system settings
    printf( "rceProxy settings:\n"
      "  Platform %d  RceIp %s  Detector %s id %d  Device %s id %d\n",
      iPlatform, sRceIp.c_str(), DetInfo::name(detector), iDetectorId, DetInfo::name(device), iDeviceId );
    
    // need to put in new numbers in DetInfo.hh for the rceProxy
    const DetInfo detInfo( getpid(), detector, iDetectorId, device, iDeviceId);    

    Task* task = new Task(Task::MakeThisATask);

    // keep this: it's the "hook" into the configuration database
    CfgClientNfs cfgService = CfgClientNfs(detInfo);
    SegWireSettingsRceProxy settings(detInfo);
    
    EvtCBRceProxy evtCbRceProxy(task, iPlatform, cfgService, sRceIp, iNumLinks, iPayloadSizePerLink, iDebugLevel);
    SegmentLevel seglevel(iPlatform, settings, evtCbRceProxy, NULL);    
    evtCbRceProxy.setSelfNode( seglevel.header() );    
    
    seglevel.attach();    
    if ( evtCbRceProxy.IsAttached() )    
        task->mainLoop();          
    
    task->destroy();     
    return 0;
}
