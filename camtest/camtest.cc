#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/client/Browser.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/RingPoolW.hh"
#include "pds/service/Task.hh"

#include "pds/camera/Opal1000.hh"
#include "FrameServer.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;
static unsigned detid = 1;

static void *thread_signals(void*)
{
  while(1) sleep(100);
}

//
//  The signal handler requires these static variables
//
static int _nsignals=0;
static Pds::FrameServer* _frameServer;
static void cameraSignalHandler(int arg)
{
  _nsignals++;
  _frameServer->post();
}


namespace Pds {

  //
  //  Sample feature extraction.
  //  Iterate through the input datagram to strip out the BLD
  //  and copy/splice the remaining data.
  //
  class MyIterator : public XtcIterator {
  public:
    MyIterator(Datagram& dg) : _dg(dg) {}
    int process(const Xtc& xtc,
		InDatagramIterator* iter)
    {
      if (xtc.contains==TypeNum::Id_Xtc)
	return iterate(xtc,iter);
      if (xtc.src.phy() == detid) {
	_dg.xtc.damage.increase(xtc.damage.value());
	memcpy(_dg.xtc.alloc(sizeof(Xtc)),&xtc,sizeof(Xtc));
	int size = xtc.sizeofPayload();
	iovec iov[1];
	int remaining = size;
	while( remaining ) {
	  int sz = iter->read(iov,1,size);
	  if (!sz) break;
	  memcpy(_dg.xtc.alloc(sz),iov[0].iov_base,sz);
	  remaining -= sz;
	} 
	return size-remaining;
      }
      return 0;
    }
  private:
    Datagram& _dg;
  };

  class L1Action : public Action {
  private:
    enum { PoolSize = 32*1024*1024 };
    enum { EventSize = 4*1024*1024 };
  public:
    L1Action() : _pool(PoolSize,EventSize), 
		 _iter(sizeof(ZcpDatagramIterator),32) {}
    ~L1Action() {}

    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* input)
    {
      if (verbose) {
	InDatagramIterator* in_iter = input->iterator(&_iter);
	int advance;
	Browser browser(input->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }
      return input;

      if (input->datagram().seq.type()   !=Sequence::Event ||
	  input->datagram().seq.service()!=TransitionId::L1Accept) {
	if (verbose) {
	  printf("events %p  type %d service %d\n",
		 input,
		 input->datagram().seq.type(),
		 input->datagram().seq.service());
	  InDatagramIterator* in_iter = input->iterator(&_iter);
	  int advance;
	  Browser browser(input->datagram(), in_iter, 0, advance);
	  browser.iterate();
	  delete in_iter;
	}
	return input;
      }

      CDatagram* ndg = new (&_pool)CDatagram(input->datagram());

      if (verbose) {
	printf("new cdatagram %p\n",ndg);
	InDatagramIterator* in_iter = input->iterator(&_iter);
	int advance;
	Browser browser(input->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }
      {
      InDatagramIterator* in_iter = input->iterator(&_iter);
      MyIterator iter(const_cast<Datagram&>(ndg->datagram()));
      iter.iterate(input->datagram().xtc,in_iter);
      delete in_iter;
      }
      if (verbose) {
	InDatagramIterator* in_iter = ndg->iterator(&_iter);
	int advance;
	Browser browser(ndg->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }

      _pool.shrink(ndg, ndg->datagram().xtc.sizeofPayload()+sizeof(Datagram));
      return ndg;
    }
  private:
    RingPoolW   _pool;
    GenericPool _iter;
  };

  class Config {
  public:
    Config(Opal1000& camera) :
      _camera(camera),
      _sig(-1)
    {
      const int fps = 4;
      _Config.Mode = Camera::MODE_EXTTRIGGER_SHUTTER;
      //      _Config.Mode = Camera::MODE_CONTINUOUS;
      _Config.Format = Pds::Frame::FORMAT_GRAYSCALE_8;
      _Config.GainPercent = 20;
      _Config.BlackLevelPercent = 5;
      _Config.ShutterMicroSec = 1000000/fps - 50;
      _Config.FramesPerSec = fps;
    }
    ~Config() 
    {
    }

    Transition* configure(Transition* tr)
    {
      printf("Configuring ...\n");
      _damage = (1<<Damage::UserDefined);
      int ret;
      if ((ret = _camera.SetConfig(_Config)) >= 0)
	if ((_sig = _camera.SetNotification(Camera::NOTIFYTYPE_SIGNAL)) >= 0) {

	  printf("Registering handler for signal %d ...\n", _sig);

	  struct sigaction action;
	  action.sa_handler = cameraSignalHandler;
	  sigemptyset(&action.sa_mask);
	  action.sa_flags   = SA_RESTART;
	  sigaction(_sig,&action,NULL);

	  printf("Initializing ...\n");

	  if ((ret = _camera.Init()) >= 0) {

	    printf("Starting ...\n");

	    if ((ret = _camera.Start()) >= 0)
	      _damage = 0;
	    else
	      printf("Camera::Start: %s.\n", strerror(-ret));
	  }
	  else
	    printf("Camera::Init: %s.\n", strerror(-ret));
	}
	else
	  printf("Camera::SetNotification: %s.\n", strerror(-_sig));
      else
	printf("Camera::SetConfig: %s.\n", strerror(-ret));

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
    Camera::Config _Config;
    Opal1000&      _camera;
    int            _sig;
    unsigned       _damage;
  };

  class ConfigAction : public Action {
  public:
    ConfigAction(Config& config) : _config(config) {}
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
    Config& _config;
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
      _camera(new Opal1000),
      _config(new Config(*_camera)),
      _server(new FrameServer(src,*_camera))
    {
      _frameServer = _server; // static for signal handler
    }

    virtual ~SegTest()
    {
      delete _server;
      delete _config;
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
      fsm->callback(TransitionId::Configure  , new ConfigAction  (*_config));
      fsm->callback(TransitionId::Unconfigure, new UnconfigAction(*_config));
      fsm->callback(TransitionId::L1Accept , new L1Action);
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
    Opal1000*     _camera;
    Config*       _config;
    FrameServer*  _server;
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
  SegTest* segtest = new SegTest(task, 
				 platform, 
				 Src(Node(Level::Source,platform), detid));
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
