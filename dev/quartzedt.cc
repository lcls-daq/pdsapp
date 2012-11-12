#include "pdsapp/dev/CmdLineTools.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pds/camera/FrameServer.hh"
#include "pds/camera/QuartzManager.hh"
#include "pds/camera/QuartzCamera.hh"
#include "pds/camera/EdtPdvCL.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;
static Pds::QuartzCamera::CLMode _mode = Pds::QuartzCamera::Full;

static void usage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform> -g <grabberId> -v\n",p);
  printf("<detinfo> = integer/integer/integer or string/integer/string/integer (e.g. XppEndStation/0/Quartz4A150/1 or 22/0/1)\n");
}

static Pds::CameraDriver* _driver(int id, const Pds::Src& src)
{
  return new Pds::EdtPdvCL(*new Pds::QuartzCamera(static_cast<const Pds::DetInfo&>(src),_mode),
                           0,id);
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
      _quartz  (new QuartzManager(src)),
      _grabberId(grabberId)
    {
      _sources.push_back(_quartz->server().client());
    }

    virtual ~SegTest()
    {
      delete _quartz;
      _task->destroy();
    }

  public:    
    // Implements SegWireSettings
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface)
    {
      wire.add_input(&_quartz->server());
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
      _quartz->appliance().connect(frmk->inlet());
      //      (new Decoder)->connect(frmk->inlet());

      _quartz->attach(_driver(_grabberId,_sources.front()));
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
      
      _quartz->detach();

      delete this;
    }
    
  private:
    Task*          _task;
    unsigned       _platform;
    QuartzManager* _quartz;
    int            _grabberId;
    std::list<Src> _sources;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const unsigned NO_PLATFORM = ~0;
  unsigned platform = NO_PLATFORM;
  Arp* arp = 0;

  DetInfo info;

  unsigned grabberId(0);

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:g:vBMF")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
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
    case 'B': _mode = Pds::QuartzCamera::Base  ; break;
    case 'M': _mode = Pds::QuartzCamera::Medium; break;
    case 'F': _mode = Pds::QuartzCamera::Full  ; break;
    }
  }

  if (platform == NO_PLATFORM) {
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
