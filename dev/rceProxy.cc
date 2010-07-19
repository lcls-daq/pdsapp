#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string>
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

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
  class EvtCbRceProxy : public EventCallback
  {
    public:
      EvtCbRceProxy(Task* task, int iPlatform, CfgClientNfs& cfgService, const string& sRceIp, const string& sConfigFile,
          TypeId typeIdData, DetInfo::Device device, int iTsWidth, int iPhase, int iDebugLevel) :
            _task(task), _iPlatform(iPlatform), _cfg(cfgService), _sRceIp(sRceIp), _sConfigFile(sConfigFile),
            _typeIdData(typeIdData), _device(device), _iTsWidth(iTsWidth), _iPhase(iPhase), _iDebugLevel(iDebugLevel),
            _bAttached(false), _rceProxymgr(NULL), _pSelfNode(NULL)
            {
            }

      virtual ~EvtCbRceProxy()
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
        printf("EvtCbRceProxy connected to iPlatform 0x%x\n", 
            _iPlatform);

        Stream* frmk = streams.stream(StreamParams::FrameWork);

        reset();                
        _rceProxymgr = new RceProxyManager(_cfg, _sRceIp, _sConfigFile, _typeIdData, _device,
            _iTsWidth, _iPhase, *_pSelfNode, _iDebugLevel);
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
      string              _sConfigFile;
      TypeId              _typeIdData;
      DetInfo::Device     _device;
      int                 _iTsWidth;
      int                 _iPhase;
      int                 _iDebugLevel;
      bool                _bAttached;
      RceProxyManager*    _rceProxymgr;
      const Node*         _pSelfNode;
  }; // class EvtCbRceProxy


} // namespace Pds 


using namespace Pds;

static void showUsage()
{
  printf( "Usage: rceProxy [-x|--version] [-h|--help] [-d|--debug <debug level>] "
      "[-t|--detector <detector type>] [-i|--detid <detector id>] "
      "[-v|--device <device type>] NB "
      "[-e|--devid <device id>] [-y|--typeid <type id>] "
      "[-s|--typever <type version>] [-w|--tswidth  <traffic shaping width>] [-z|--phase <phase>] "
      "-p|--platform <platform id>  -r|--rceip <rce ip> \n" 
      "  Options:\n"
      "    -x|--version       Show file version\n"
      "    -h|--help          Show Usage\n"
      "    -d|--debug         Set debug level\n"
      "    -t|--detector      Set detector type by name [Default: Camp]\n"
      "    -i|--detid         Set detector id           [Default: 0]\n"
      "    -v|--device        Set device type by name   [Default: pnCCD]\n"
      "    -e|--devid         Set device id             [Default: 0]\n"
      "    -y|--typeid        Set data type id by name  [Default: Id_pnCCDframe]\n"
      "    -s|--typever       Set data type version     [Default: 1]\n"
      "    -w|--tswidth       Set traffic shaping width [Default: 4]\n"
      "    -z|--phase         Set initial phase         [Default: 0]\n"
      "    -p|--platform      [*required*] Set platform id \n"
      "    -r|--rceip         [*required*] Set host IP address of RCE \n"
      "    -f|--cfgFile       [*required*] Set the config file path and name \n"
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
      {"typeid",       1, 0, 'y'},
      {"typever",      1, 0, 's'},
      {"tswidth",      1, 0, 'w'},
      {"phase",        1, 0, 'z'},
      {"platform",     1, 0, 'p'},
      {"rceIp",        1, 0, 'r'},
      {"cfgFile",      1, 0, 'f'},
      {0,              0, 0,  0 },
  };

  // parse the command line for our boot parameters
  int                 iDebugLevel         = 0;
  DetInfo::Detector   detector            = DetInfo::Camp;
  int                 iDetectorId         = 0;
  DetInfo::Device     device              = DetInfo::pnCCD;
  int                 iDeviceId           = 0;
  TypeId::Type        typeId              = TypeId::Id_pnCCDframe;
  int                 iTypeVer            = 1;
  int                 iTsWidth            = 4;
  int                 iPhase              = 0;
  int                 iPlatform           = -1;
  string              sRceIp;
  string              sConfigFile;
  bool                found;

  while ( int opt = getopt_long(argc, argv, ":xhd:t:i:v:y:e:w:z:p:r:f:", loOptions, &iOptionIndex ) )
  {
    if ( opt == -1 ) break;
    unsigned index;

    switch(opt)
    {
      case 'x':
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
        found = false;
        for (index=0; index<DetInfo::NumDetector; index++) {
          if (!strcmp(optarg, DetInfo::name((DetInfo::Detector)index))) {
            detector = (DetInfo::Detector)index;
            found = true;
          }
        }
        if (!found) {
          printf("Bad Detector name: %s\n  Detector Names are:\n", optarg);
          for (index=0; index<DetInfo::NumDetector; index++) {
            printf("\t%s\n", DetInfo::name((DetInfo::Detector)index));
          }
        }
        break;
      case 'v':
        found = false;
        for (index=0; index<DetInfo::NumDevice; index++) {
          if (!strcmp(optarg, DetInfo::name((DetInfo::Device)index))) {
            device = (DetInfo::Device)index;
            found = true;
          }
        }
        if (!found) {
          printf("Bad Device name: %s\n  Device Names are:\n", optarg);
          for (index=0; index<DetInfo::NumDevice; index++) {
            printf("\t%s\n", DetInfo::name((DetInfo::Device)index));
          }
        }
        break;
      case 'y':
        typeId = (TypeId::Type) strtoul(optarg, NULL, 0);
        found = false;
        for (index=0; index<TypeId::NumberOf; index++) {
          if (!strcmp(optarg, TypeId::name((TypeId::Type)index))) {
            typeId = (TypeId::Type)index;
            found = true;
          }
        }
        if (!found) {
          printf("Bad Type name: %s\n  Device Names are:\n", optarg);
          for (index=0; index<TypeId::NumberOf; index++) {
            printf("\t%s\n", TypeId::name((TypeId::Type)index));
          }
        }
        break;
      case 'i':
        iDetectorId = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        iDeviceId = strtoul(optarg, NULL, 0);
        break;
      case 's':
        iTypeVer = strtoul(optarg, NULL, 0);
        break;
      case 'w':
        iTsWidth = strtoul(optarg, NULL, 0);
        break;
      case 'z':
        iPhase = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        iPlatform = strtol(optarg, NULL, 0);
        break;
      case 'r':
        sRceIp = optarg;
        break;
      case 'f':
        sConfigFile = optarg;
        break;
      case ':':               /* Terse output mode */
        printf( "rceProxy:main(): Missing argument for %c\n", optopt );
        break;
      case '?':               /* Terse output mode */
        printf( "rceProxy:main(): Unknown option: %c\n", optopt );
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
  if (!device) {
    printf("rceProxy:main(): Please specify a device, pnCCD or CSPAD\n");
    showUsage();
    return 2;
  }
  if ( sConfigFile.empty() && (device==DetInfo::pnCCD))
  {
    printf( "rceProxy:main(): Please specify pnCCD Config File in command line\n" );
    showUsage();
    return 2;
  }

  // print summary of command line options and system settings
  printf( "rceProxy settings:\n"
      "  Platform %d  RceIp %s  Detector %s id %d  Device %s id %d  Type id %s ver %d\n",
      iPlatform, sRceIp.c_str(), DetInfo::name(detector), iDetectorId, DetInfo::name(device), iDeviceId, 
      TypeId::name(typeId), iTypeVer );
  if (device==DetInfo::pnCCD) printf( "/tRCE pnCCD config file:%s\n", sConfigFile.c_str());

  TypeId typeIdData(typeId, iTypeVer);

  // need to put in new numbers in DetInfo.hh for the rceProxy
  const DetInfo detInfo( getpid(), detector, iDetectorId, device, iDeviceId);

  Task* task = new Task(Task::MakeThisATask);

  // keep this: it's the "hook" into the configuration database
  CfgClientNfs cfgService = CfgClientNfs(detInfo);
  SegWireSettingsRceProxy settings(detInfo);

  EvtCbRceProxy evtCbRceProxy(task, iPlatform, cfgService, sRceIp, sConfigFile,
      typeIdData, device, iTsWidth, iPhase, iDebugLevel);
  SegmentLevel seglevel(iPlatform, settings, evtCbRceProxy, NULL);
  evtCbRceProxy.setSelfNode( seglevel.header() );

  seglevel.attach();
  if ( evtCbRceProxy.IsAttached() )
    task->mainLoop(); // Enter the event processing loop, and never returns (unless the program terminates)

  return 0;
}
