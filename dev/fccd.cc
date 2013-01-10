#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pds/camera/FccdManager.hh"
#include "pds/camera/FccdCamera.hh"
#include "pds/camera/PicPortCL.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static bool verbose = false;

static void usage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform> [-v] [-h]\n",p);
}

static void help()
{
  printf("Options:\n"
         "  -i <detinfo>          integer/integer/integer\n"
         "  -p <platform>         platform number\n"
         "  -v                    be verbose (default=false)\n"
         "  -h                    help: print this message and exit\n");
}

static Pds::CameraDriver* _driver()
{
  return new PdsLeutron::PicPortCL(*new Pds::FccdCamera, 0, "Stereo");
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
	    const Src&            src) :
      _task    (task),
      _platform(platform),
      _fccd  (new FccdManager(src))
    {
      _sources.push_back(_fccd->server().client());
    }

    virtual ~SegTest()
    {
      delete _fccd;
      _task->destroy();
    }

  public:    
    // Implements SegWireSettings
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface)
    {
      wire.add_input(&_fccd->server());
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
      _fccd->appliance().connect(frmk->inlet());
      //      (new Decoder)->connect(frmk->inlet());

      _fccd->attach(_driver());
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
      
      _fccd->detach();

      delete this;
    }
    
  private:
    Task*          _task;
    unsigned       _platform;
    FccdManager* _fccd;
    std::list<Src> _sources;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = -1UL;
  Arp* arp = 0;

  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  extern char* optarg;
  char* endPtr;
  int c;
  bool helpFlag = false;
  bool infoFlag = false;
  while ( (c=getopt( argc, argv, "a:i:p:vh")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      errno = 0;
      endPtr = NULL;
      det    = (DetInfo::Detector)strtoul(optarg, &endPtr, 0);
      if (errno || (endPtr == NULL) || (*endPtr != '/')) {
        printf("Error: failed to parse detinfo\n");
        usage(argv[0]);
        return -1;
      }
      detid  = strtoul(endPtr+1, &endPtr, 0);
      if (errno || (endPtr == NULL) || (*endPtr != '/')) {
        printf("Error: failed to parse detinfo\n");
        usage(argv[0]);
        return -1;
      }
      devid  = strtoul(endPtr+1, &endPtr, 0);
      if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
        printf("Error: failed to parse detinfo\n");
        usage(argv[0]);
        return -1;
      }
      infoFlag = true;
      break;
    case 'p':
      errno = 0;
      endPtr = NULL;
      platform = strtoul(optarg, &endPtr, 0);
      if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
        printf("Error: failed to parse platform number\n");
        usage(argv[0]);
        return -1;
      }
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

  SegTest* segtest = new SegTest(task, 
				 platform, 
				 DetInfo(node.pid(), 
					 det, detid, 
					 DetInfo::Fccd, devid));

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
