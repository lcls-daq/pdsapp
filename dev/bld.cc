#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/EventStreams.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/BldServer.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;

static const unsigned NetBufferDepth = 32;

namespace Pds {

  class BldCallback : public EventCallback, 
		      public SegWireSettings {
  public:
    BldCallback(Task*      task,
		unsigned   platform,
		unsigned   mask) :
      _task    (task),
      _platform(platform),
      _mask    (mask)
    {
      Node node(Level::Source,platform);
      _sources.push_back(DetInfo(node.pid(), 
				 DetInfo::BldEb, 0,
				 DetInfo::NoDevice, 0));
    }

    virtual ~BldCallback() { _task->destroy(); }

  public:    
    // Implements SegWireSettings
    void connect (InletWire& inlet, StreamParams::StreamType s, int ip) 
    {
      for(int i=0; i<BldInfo::NumberOf; i++) {
	if (_mask & (1<<i)) {
	  Node node(Level::Reporter, 0);
	  node.fixup(StreamPorts::bld(i).address(),Ether());
	  Ins ins( node.ip(), StreamPorts::bld(0).portId());
	  BldServer* srv = new BldServer(ins, node.procInfo(), NetBufferDepth);
	  inlet.add_input(srv);
	  srv->server().join(ins, ip);
	  printf("Bld::allocated assign bld  fragment %d  %x/%d\n",
		 srv->id(),ins.address(),srv->server().portId());
	}
      } 
    }
    const std::list<Src>& sources() const { return _sources; }

  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams) {}
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
    unsigned       _mask;
    std::list<Src> _sources;
  };

  class BldSegmentLevel : public SegmentLevel {
  public:
    BldSegmentLevel(unsigned     platform, 
		    BldCallback& cb) :
      SegmentLevel(platform, cb, cb, 0) {}
    ~BldSegmentLevel() {}
  public:
    bool attach() {
      start();
      if (connect()) {
	_streams = new EventStreams(*this);  // specialized here
	_streams->connect();

	_callback.attached(*_streams);

	//  Add the L1 Data servers  
	_settings.connect(*_streams->wire(StreamParams::FrameWork),
			  StreamParams::FrameWork, 
			  header().ip());

	//    Message join(Message::Ping);
	//    mcast(join);
	mcast(_reply);
	return true;
      } else {
	_callback.failed(EventCallback::PlatformUnavailable);
	return false;
      }
    }
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = 0;
  unsigned mask = (1<<BldInfo::NumberOf)-1;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "p:m:")) != EOF ) {
    switch(c) {
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'm':
      mask = strtoul(optarg, NULL, 0);
      break;
    default:
      break;
    } 
 }

  if (!platform) {
    printf("%s: -p <platform> required\n",argv[0]);
    return 0;
  }

  Task*               task = new Task(Task::MakeThisATask);
  BldCallback*          cb = new BldCallback(task, platform, mask);
  BldSegmentLevel* segment = new BldSegmentLevel(platform, *cb);
  if (segment->attach())
    task->mainLoop();

  segment->detach();
  return 0;
}
