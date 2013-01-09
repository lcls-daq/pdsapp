#include "pdsapp/dev/CmdLineTools.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/utility/ToEventWireScheduler.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include "pds/camera/FrameServer.hh"
#include "pds/camera/Opal1kManager.hh"
#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/PicPortCL.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;

static void usage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform> [-g <grabberId>] [-v] [-h]\n",p);
}

static void help()
{
  printf("Options:\n"
         "  -i <detinfo>          integer/integer/integer or string/integer/string/integer\n"
         "                          (e.g. XppEndStation/0/Opal1000/1 or 22/0/1)\n"
         "  -p <platform>         platform number\n"
         "  -g <grabberId>        grabber ID (default=0)\n"
         "  -v                    be verbose (default=false)\n"
         "  -h                    help: print this message and exit\n");
}

static Pds::CameraDriver* _driver(int id, const Pds::Src& src) 
{
  return new PdsLeutron::PicPortCL(*new Pds::Opal1kCamera(static_cast<const Pds::DetInfo&>(src)),id);
}

static void *thread_signals(void*)
{
  while(1) sleep(10);
  return 0;
}

namespace Pds {

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class SegTest : public EventCallback, public SegWireSettings {
  public:
    SegTest(Task*                 task,
	    unsigned              platform,
	    const Src&            src,
	    unsigned              grabberId) :
      _task    (task),
      _platform(platform),
      _opal1k  (new Opal1kManager(src)),
      _grabberId(grabberId)
    {
      _sources.push_back(_opal1k->server().client());
    }

    virtual ~SegTest()
    {
      delete _opal1k;
      _task->destroy();
    }

  public:    
    // Implements SegWireSettings
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface)
    {
      wire.add_input(&_opal1k->server());
    }

    const std::list<Src>& sources() const
    {
      return _sources;
    }

  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("SegTest connected to platform 0x%x\n", 
	     _platform);

      Stream* frmk = streams.stream(StreamParams::FrameWork);
      _opal1k->appliance().connect(frmk->inlet());
      //      (new Decoder)->connect(frmk->inlet());

      _opal1k->attach(_driver(_grabberId,_sources.front()));
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
      
      _opal1k->detach();

      delete this;
    }
    
  private:
    Task*          _task;
    unsigned       _platform;
    Opal1kManager* _opal1k;
    int            _grabberId;
    std::list<Src> _sources;
  };
}

using namespace Pds;


int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = -1UL;
  Arp* arp = 0;

  DetInfo info;
  bool infoFlag = false;
  bool helpFlag = false;

  unsigned grabberId(0);

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:g:vh")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
        usage(argv[0]);
        return -1;
      } else {
        infoFlag = true;
      }
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'g':
      grabberId = strtoul(optarg, NULL, 0);
      break;
    case 'v':
      verbose = true;
      break;
    case 'h':
      helpFlag = true;
      break;
    }
  }

  if (helpFlag) {
    usage(argv[0]);
    help();
    return 0;
  } else if (!infoFlag || (platform == -1UL)) {
    printf("Error: Platform and detinfo required\n");
    usage(argv[0]);
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
  info = DetInfo(node.pid(), info.detector(), info.detId(), info.device(), info.devId());

  SegTest* segtest = new SegTest(task, 
				 platform,
                                 info,
				 grabberId);

  if (info.device()==DetInfo::Opal4000)
    ToEventWireScheduler::setMaximum(3);

  printf("Creating segment level ...\n");
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
