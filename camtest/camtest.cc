#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/client/Browser.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/RingPoolW.hh"
#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

#include "pds/camera/Camera.hh"
#include "pds/camera/DmaSplice.hh"
#include "pds/camera/Opal1000.hh"

#include "pds/config/CfgClientNfs.hh"

#include "pds/diagnostic/Profile.hh"

#include "FexFrameServer.hh"
#include "CameraFexConfig.hh"
#include "Opal1kConfig.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;
static unsigned detid = 1;

static void *thread_signals(void*)
{
  while(1) sleep(100);
  return 0;
}

//
//  The signal handler requires these static variables
//
static int _nsignals=0;
static Pds::FexFrameServer* _frameServer;

static void cameraSignalHandler(int arg)  // arg is the signal number
{
  _nsignals++;
  _frameServer->post();
}


namespace Pds {

  class Config {
    enum { MaxMaskedPixels=100 };
  public:
    Config(const Src& src,
	   PdsLeutron::Opal1000&  camera,
	   DmaSplice& splice,
	   FexFrameServer& action) :
      _camera(camera),
      _splice(splice),
      _l1action(action),
      _sig(-1),
      _configBuffer(new char[sizeof(Opal1kConfig) + 
			     sizeof(CameraFexConfig)+100*sizeof(CameraPixelCoord)]),
      _configService(src)
    {
    }
    ~Config() 
    {
      delete[] _configBuffer;
    }

    Transition* allocate (Transition* tr)
    {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _configService.initialize(alloc);
      return tr;
    }

    Transition* configure(Transition* tr)
    {
      printf("Configuring ...\n");
      _damage = (1<<Damage::UserDefined);
      //
      //  retrieve the configuration
      //
#if 1
      char* cfgBuff = _configBuffer;
      int len;
      if ((len=_configService.fetch(*tr,TypeId::Id_Opal1kConfig, cfgBuff)) <= 0) {
	printf("Config::configure failed to retrieve Opal1000 configuration\n");
	return tr;
      }
      const Opal1kConfig&   opalConfig = *new(cfgBuff) Opal1kConfig;
      cfgBuff += len;

      if ((len=_configService.fetch(*tr,TypeId::Id_CameraFexConfig, cfgBuff)) <= 0) {
	printf("Config::configure failed to retrieve Opal1000 configuration\n");
	return tr;
      }
      const CameraFexConfig& fexConfig = *new(cfgBuff) CameraFexConfig;
#else
      //
      //  (static for now)
      //
      Opal1kConfig opalConfig;
      opalConfig.Depth_bits         = 12;
      opalConfig.Gain_percent       = 20;
      opalConfig.BlackLevel_percent = 0;
      opalConfig.ShutterWidth_us    = 540;

      CameraFexConfig& fexConfig = *new (_configBuffer)CameraFexConfig;
      //      fexConfig.algorithm     = CameraFexConfig::RawImage;
      //      fexConfig.algorithm     = CameraFexConfig::RegionOfInterest;
      fexConfig.algorithm     = CameraFexConfig::TwoDGaussian;
      //      fexConfig.algorithm     = CameraFexConfig::TwoDGaussianAndFrame;
      fexConfig.regionOfInterestStart.column = 300;
      fexConfig.regionOfInterestStart.row    =  50;
      fexConfig.regionOfInterestEnd  .column = 700;
      fexConfig.regionOfInterestEnd  .row    = 450;
      fexConfig.nMaskedPixels = 0;
#endif
      _l1action.Config(fexConfig);

      PdsLeutron::Camera::Config Config;
      Config.Mode              = PdsLeutron::Camera::MODE_EXTTRIGGER_SHUTTER;
      switch(opalConfig.Depth_bits) {
      case  8: Config.Format            = PdsLeutron::FrameHandle::FORMAT_GRAYSCALE_8 ; break;
      case 10: Config.Format            = PdsLeutron::FrameHandle::FORMAT_GRAYSCALE_10; break;
      case 12:
      default: Config.Format            = PdsLeutron::FrameHandle::FORMAT_GRAYSCALE_12; break;
      }
      Config.GainPercent       = opalConfig.Gain_percent;
      Config.BlackLevelPercent = opalConfig.BlackLevel_percent;
      Config.ShutterMicroSec   = opalConfig.ShutterWidth_us;
      Config.FramesPerSec      = 5;

      int ret;
      if ((ret = _camera.SetConfig(Config)) < 0)
	printf("Camera::SetConfig: %s.\n", strerror(-ret));

      else if ((_sig = _camera.SetNotification(PdsLeutron::Camera::NOTIFYTYPE_SIGNAL)) < 0) 
	printf("Camera::SetNotification: %s.\n", strerror(-_sig));

      else {
	printf("Registering handler for signal %d ...\n", _sig);
	struct sigaction action;
	action.sa_handler = cameraSignalHandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags   = SA_RESTART;
	sigaction(_sig,&action,NULL);
	
	printf("Initializing ...\n");
	if ((ret = _camera.Init()) < 0)
	  printf("Camera::Init: %s.\n", strerror(-ret));
	
	else {
	  printf("DmaSplice::initialize %p 0x%x\n", 
		 _camera.FrameBufferBaseAddress,
		 _camera.FrameBufferEndAddress - _camera.FrameBufferBaseAddress);
	  _splice.initialize( _camera.FrameBufferBaseAddress,
			      _camera.FrameBufferEndAddress - _camera.FrameBufferBaseAddress);
	  
	  printf("Starting ...\n");
	  if ((ret = _camera.Start()) < 0)
	    printf("Camera::Start: %s.\n", strerror(-ret));
	  else
	    _damage = 0;
	}
      }

      printf("Done\n");
      return tr;
    }

    Transition* unconfigure(Transition* tr)
    {
      _camera.Stop();
      struct sigaction action;
      action.sa_handler = SIG_DFL;
      sigaction(_sig,&action,NULL);
      return tr;
    }

    InDatagram* configure  (InDatagram* in) 
    {
      const_cast<Damage&>(in->datagram().xtc.damage).increase(_damage); 
      return in;
    }
    InDatagram* unconfigure(InDatagram* in) { return in; }

  private:
    PdsLeutron::Opal1000& _camera;
    DmaSplice&            _splice;
    FexFrameServer&       _l1action;
    int                   _sig;
    char*                 _configBuffer;
    CfgClientNfs          _configService;
    unsigned              _damage;
  };

  class MapAction : public Action {
  public:
    MapAction(Config&   config) : _config(config) {}
    Transition* fire(Transition* tr) { return _config.allocate(tr); }
    InDatagram* fire(InDatagram* dg) { return dg; }
  private:
    Config& _config;
  };

  class ConfigAction : public Action {
  public:
    ConfigAction(Config&   config) : _config(config) {}
    Transition* fire(Transition* tr) { return _config.configure(tr); }
    InDatagram* fire(InDatagram* dg) { return _config.configure(dg); }
  private:
    Config& _config;
  };

  class UnconfigAction : public Action {
  public:
    UnconfigAction(Config& config) : _config(config) {}
    Transition* fire(Transition* tr) { return _config.unconfigure(tr); }
    InDatagram* fire(InDatagram* dg) { return _config.unconfigure(dg); }
  private:
    Config&   _config;
  };

  class BeginRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { Profile::initialize(); return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };

  class EndRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { Profile::finalize(); return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class SegTest : public EventCallback, public SegWireSettings {
  public:
    SegTest(Task*                 task,
	    unsigned              platform,
	    const Src&            src) :
      _task    (task),
      _platform(platform),
      _splice(new DmaSplice),
      _camera(new PdsLeutron::Opal1000),
      _server(new FexFrameServer(src,*_camera,*_splice)),
      _config(new Config(src,*_camera,*_splice,*_server))
    {
      _frameServer = _server; // static for signal handler
    }

    virtual ~SegTest()
    {
      delete _server;
      delete _config;
      delete _splice;
      delete _camera;
      _task->destroy();
    }

  public:    
    // Implements SegWireSettings
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface)
    {
      wire.add_input(_server);
    }

  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("SegTest connected to platform 0x%x\n", 
	     _platform);

      Stream* frmk = streams.stream(StreamParams::FrameWork);
      Fsm* fsm = new Fsm;
      fsm->callback(TransitionId::Map        , new MapAction     (*_config));
      fsm->callback(TransitionId::Configure  , new ConfigAction  (*_config));
      fsm->callback(TransitionId::BeginRun   , new BeginRunAction);
      fsm->callback(TransitionId::EndRun     , new EndRunAction);
      fsm->callback(TransitionId::Unconfigure, new UnconfigAction(*_config));
      fsm->connect(frmk->inlet());
      //      (new Decoder)->connect(frmk->inlet());
    }
    void failed(Reason reason)
    {
      static const char* reasonname[] = { "platform unavailable", 
					  "crates unavailable", 
					  "fcpm unavailable" };
      printf("SegTest: unable to allocate crates on platform 0x%x : %s\n", 
	     _platform, reasonname[reason]);
      delete this;
    }
    void dissolved(const Node& who)
    {
      const unsigned userlen = 12;
      char username[userlen];
      Node::user_name(who.uid(),username,userlen);
      
      const unsigned iplen = 64;
      char ipname[iplen];
      Node::ip_name(who.ip(),ipname, iplen);
      
      printf("SegTest: platform 0x%x dissolved by user %s, pid %d, on node %s", 
	     who.platform(), username, who.pid(), ipname);
      
      delete this;
    }
    
  private:
    Task*         _task;
    unsigned      _platform;
    DmaSplice*    _splice;
    PdsLeutron::Opal1000* _camera;
    FexFrameServer* _server;
    Config*       _config;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = 0;
  Arp* arp = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:v")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      detid  = strtoul(optarg, NULL, 0);
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'v':
      verbose = true;
      break;
    }
  }

  if (!platform) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }

  // launch the SegmentLevel
  if (arp) {
    if (arp->error()) {
      char message[128];
      sprintf(message, "failed to create Arp : %s", 
	      strerror(arp->error()));
      printf("%s %s\n",argv[0], message);
      delete arp;
      return 0;
    }
  }

  printf("Starting handler thread ...\n");
  pthread_t h_thread_signals;
  pthread_create(&h_thread_signals, NULL, thread_signals, 0);

  //
  // Block all signals from this thread (and daughters)
  //
  sigset_t sigset_full;
  sigfillset(&sigset_full);
  pthread_sigmask(SIG_BLOCK, &sigset_full, 0);

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,platform);
  SegTest* segtest = new SegTest(task, 
				 platform, 
				 DetInfo(node.pid(), DetInfo::AmoXes, detid, DetInfo::Opal1000, 0));
  SegmentLevel* segment = new SegmentLevel(platform, 
					   *segtest,
					   *segtest, 
					   arp);
  if (segment->attach()) {
    task->mainLoop();
  }

  segment->detach();
  if (arp) delete arp;
  return 0;
}
