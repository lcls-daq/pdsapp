#include "pds/collection/CollectionManager.hh"

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
#include "pds/camera/Frame.hh"
#include "pds/camera/Opal1000.hh"
#include "pds/camera/TwoDMoments.hh"
#include "pds/camera/TwoDGaussian.hh"

#include "pds/config/CfgClientNfs.hh"

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

typedef unsigned short pixel_type;

class Fex {
public:
  Fex() 
  {
    _frame_buffer = new char[2*1024*1024];
  }

  void setCamera(PdsLeutron::Opal1000& camera) { _camera = &camera; }

void post() 
{
  PdsLeutron::FrameHandle* handle = _camera->GetFrameHandle();
  Pds::Frame frame(*handle);

  pixel_type* input = reinterpret_cast<unsigned short*>(handle->data);
  //  memcpy(_frame_buffer, handle->data, frame.extent);
  //  pixel_type* input = reinterpret_cast<unsigned short*>(_frame_buffer);
  
  {
    unsigned long wsum = 0;
    const pixel_type* data = reinterpret_cast<unsigned short*>(input);
    for(unsigned k=0; k<frame.height; k++) {
      for(unsigned j=0; j<frame.width; j+=16,data+=16)
	wsum += *data;
    }
    _sum = wsum;
  }

  {
    const pixel_type* data = reinterpret_cast<unsigned short*>(input);
    unsigned long wsum = 0;
    for(unsigned k=0; k<frame.height; k++) {
      for(unsigned j=0; j<frame.width; j+=8,data+=8)
	wsum += *data;
    }
    _sum = wsum;
  }

  const unsigned short* data = reinterpret_cast<unsigned short*>(input);
  Pds::TwoDMoments moments(frame.width, frame.height, data);
  
  _gss = Pds::TwoDGaussian(moments);
}
private:
  unsigned long _sum;
  Pds::TwoDGaussian _gss;
  PdsLeutron::Opal1000* _camera;
  char* _frame_buffer;
};


//
//  The signal handler requires these static variables
//
static int _nsignals=0;

static Fex* _fex;

static void cameraSignalHandler(int arg)  // arg is the signal number
{
  _nsignals++;
  _fex->post();
}


namespace Pds {

  //
  //  Implementation of the transition observer 
  //    (sees the entire platform)
  //
  static const unsigned MaxPayload = sizeof(Allocate);
  static const unsigned ConnectTimeOut = 250; // 1/4 second

  class Observer : public CollectionManager {
  public:
    Observer(unsigned char platform) :
      CollectionManager(Level::Observer, platform, MaxPayload, ConnectTimeOut, NULL) {}
    ~Observer() {}
  public:
    virtual void post(const Transition&) = 0;
  private:
    void message(const Node& hdr, const Message& msg)
    {
      if (hdr.level() == Level::Control && msg.type()==Message::Transition) {
        const Transition& tr = reinterpret_cast<const Transition&>(msg);
	if (tr.phase() == Transition::Execute)
	  post(tr);
      }
    }
  };

  class CamObserver : public Observer {
  public:
    CamObserver(const Src&            src,
		unsigned              platform) : 
      Observer      (platform), 
      _camera      (*new PdsLeutron::Opal1000),
      _splice      (*new DmaSplice),
      _configBuffer(new char[sizeof(Opal1kConfig) + 
			     sizeof(CameraFexConfig)+100*sizeof(CameraPixelCoord)]),
      _configService(src)
    { _fex->setCamera(_camera); }
    ~CamObserver() { delete[] _configBuffer; }

    void post(const Transition& tr) {
      Transition* ptr = const_cast<Transition*>(&tr);
      printf("CamObserver::post tr id %d\n",tr.id());
      switch(ptr->id()) {
      case TransitionId::Map:
	{
	  const Allocate& alloc = reinterpret_cast<const Allocate&>(tr);
	  _configService.initialize(alloc);
	}
	break;
      case TransitionId::Configure:
	{
	  //
	  //  retrieve the configuration
	  //
	  char* cfgBuff = _configBuffer;
	  int len;
	  if ((len=_configService.fetch(tr,TypeId::Id_Opal1kConfig, cfgBuff)) <= 0) {
	    printf("Config::configure failed to retrieve Opal1000 configuration\n");
	    return;
	  }
	  const Opal1kConfig&   opalConfig = *new(cfgBuff) Opal1kConfig;
	  cfgBuff += len;

	  if ((len=_configService.fetch(tr,TypeId::Id_CameraFexConfig, cfgBuff)) <= 0) {
	    printf("Config::configure failed to retrieve Opal1000 configuration\n");
	    return;
	  }
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
		//		_damage = 0;
		;
	    }
	  }
	  
	  printf("Done\n");
	}
	break;
      case TransitionId::Unconfigure:
	{
	  _camera.Stop();
	  struct sigaction action;
	  action.sa_handler = SIG_DFL;
	  sigaction(_sig,&action,NULL);
	}
	break;
      case TransitionId::BeginRun:
	break;
      case TransitionId::EndRun:
	break;
      default:
	break;
      }
    }
  private:
    PdsLeutron::Opal1000& _camera;
    DmaSplice&            _splice;
    //    FexFrameServer&       _l1action;
    int                   _sig;
    char*                 _configBuffer;
    CfgClientNfs          _configService;
  };

}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:p:v")) != EOF ) {
    switch(c) {
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

  printf("Starting handler thread ...\n");
  pthread_t h_thread_signals;
  pthread_create(&h_thread_signals, NULL, thread_signals, 0);

  //
  // Block all signals from this thread (and daughters)
  //
  sigset_t sigset_full;
  sigfillset(&sigset_full);
  pthread_sigmask(SIG_BLOCK, &sigset_full, 0);

  _fex = new Fex;

  Node node(Level::Source,platform);
  CamObserver observer(DetInfo(node.pid(), DetInfo::AmoXes,
			       detid, DetInfo::Opal1000, 0),
		       platform);
  observer.start();
  observer.connect();

  printf("Observer connected\n");

  Task* task = new Task(Task::MakeThisATask);
  task->mainLoop();

  return 0;
}
