#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
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
  printf("Usage: %s -i <detinfo> -p <platform> -g <grabberId> -v\n",p);
  printf("<detinfo> = integer/integer/integer or string/integer/string/integer (e.g. XppEndStation/0/Opal1000/1 or 22/0/1)\n");
}

static Pds::CameraDriver* _driver(int id) 
{
  return new PdsLeutron::PicPortCL(*new Pds::Opal1kCamera,id);
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

      _opal1k->attach(_driver(_grabberId));
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


static bool parseDetInfo(const char* args, DetInfo& info)
{
  DetInfo::Detector det(DetInfo::NumDetector);
  DetInfo::Device   dev(DetInfo::NumDevice);
  unsigned detid(0), devid(0);

  printf("Parsing %s\n",args);

  char* p;
  det    = (DetInfo::Detector)strtoul(args, &p, 0);
  if (p != args) {
    detid  = strtoul(p+1 , &p, 0);
    dev    = DetInfo::Opal1000;
    devid  = strtoul(p+1 , &p, 0);
  }
  else {
    int n = (p=strchr(args,'/')) - args;
    det = DetInfo::NumDetector;
    for(int i=0; i<DetInfo::NumDetector; i++)
      if (strncasecmp(args,DetInfo::name((DetInfo::Detector)i),n)==0) {
        det = (DetInfo::Detector)i;
        break;
      }
    if (det == DetInfo::NumDetector)
      return false;

    detid  = strtoul(p+1 , &p, 0);

    args = p+1;
    n = (p=strchr(args,'/')) - args;
    for(int i=0; i<DetInfo::NumDevice; i++)
      if (strncasecmp(args,DetInfo::name((DetInfo::Device)i),n)==0) {
        dev = (DetInfo::Device)i;
        break;
      }
    if (dev == DetInfo::NumDevice)
      return false;

    devid  = strtoul(p+1 , &p, 0);
  }

  info = DetInfo(0, det, detid, dev, devid);
  printf("Sourcing %s\n",DetInfo::name(info));
  return true;
}


int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = -1UL;
  Arp* arp = 0;

  DetInfo info;

  unsigned grabberId(0);

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:g:v")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      if (!parseDetInfo(optarg,info)) {
        usage(argv[0]);
        return -1;
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
    }
  }

  if (platform == -1UL) {
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
  info = DetInfo(node.pid(), info.detector(), info.detId(), info.device(), info.devId());

  SegTest* segtest = new SegTest(task, 
				 platform,
                                 info,
				 grabberId);

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
