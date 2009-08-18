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
#include "pds/camera/FexFrameServer.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;

static void *thread_signals(void*)
{
  while(1) sleep(100);
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
      _opal1k  (new Opal1kManager(src))
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
    Task*          _task;
    unsigned       _platform;
    Opal1kManager* _opal1k;
    std::list<Src> _sources;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = 0;
  Arp* arp = 0;

  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:v")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      det    = (DetInfo::Detector)strtoul(optarg, &endPtr, 0);
      detid  = strtoul(endPtr+1, &endPtr, 0);
      devid  = strtoul(endPtr+1, &endPtr, 0);
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
				 DetInfo(node.pid(), 
					 det, detid, 
					 DetInfo::Opal1000, devid));
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
