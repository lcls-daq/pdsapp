#include "pds/collection/CollectionManager.hh"
#include "pds/utility/Transition.hh"

#include "pds/service/Semaphore.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/management/ControlLevel.hh"
#include "pds/client/Decoder.hh"

#include "pds/service/Task.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>

namespace Pds {

  //  Add partition management to control level
  class MyControl : public ControlLevel {
  public:
    MyControl(unsigned platform, ControlCallback& cb, Arp* arp) :
      ControlLevel(platform, cb, arp),
      _members   ("Platform members",-1UL),
      _partition ("Partition",-1UL),
      _transition("Transition",-1UL)
    {
    }
    ~MyControl() {}

    void add_member(const Node& n)
    {
      _members.add(n);
    }

    void map(const Sequence& seq)
    {
      //  Equate the partition id with the platform id until we have a master
      _partition = Allocate("partition",header().platform(),seq);
      unsigned nnodes = _members.nnodes();
      for(unsigned k=0; k<nnodes; k++)
	_partition.add( *_members.node(k) );
      mcast(_partition);
    }

    void message(const Node& hdr, const Message& msg)
    {
      switch(msg.type()) {
      case Message::Ping:
      case Message::Join:
	if (!_isallocated) {
	  printf("Found node %x/%d/%d\n",hdr.ip(),hdr.level(),hdr.pid());
	    _members.add(hdr);
	}
	break;
      case Message::Transition:
	_transition.add(hdr);
	break;
      default:
	break;
      }
      PartitionMember::message(hdr,msg);
    }
  private:
    Allocate _members;
    Allocate _partition;
    Allocate _transition;
  };

  class TransitionAction : public Appliance {
  public:
    Transition* transitions(Transition*  ) { return 0; }
    InDatagram* occurrences(InDatagram* i) { return i; } 
    InDatagram* events     (InDatagram* i) { return i; }
  };
    
  class MyCallback : public ControlCallback {
  public:
    void allocated(SetOfStreams& streams) {
      //  By default, there are no external clients for this stream
      Stream& frmk = *streams.stream(StreamParams::FrameWork);
      frmk.outlet()->sink(Sequence::Event, (1<<Sequence::NumberOfServices)-1);
      (new Decoder(Level::Control))->connect(frmk.inlet());
      (new TransitionAction)->connect(frmk.inlet());
      printf("Partition allocated\n");
    }
    void failed(Reason reason) {
      printf("Partition failed to allocate: reason %d\n", reason);
    }
    void dissolved(const Node& node) {
      printf("Partition dissolved by uid %d pid %d ip %x\n",
	     node.uid(), node.pid(), node.ip());
    }
  };

  class MyEnable : public Routine {
  public:
    MyEnable(CollectionManager& control,
	     Task* task) : _control(control), 
			   _task(task), 
			   _enabled(false), 
			   _pulseId(0) 
    {
      _td.tv_sec = 0;
      _td.tv_nsec = (1<<30)/200;
    }

    void enable() { if (!_enabled) { _enabled = true; _task->call(this); } }
    void disable() { _enabled = false; }
    void routine() {
      timespec tp;
      clock_gettime(CLOCK_REALTIME, &tp);
      unsigned  sec  = tp.tv_sec;
      unsigned  nsec = tp.tv_nsec&~0x7FFFFF;
      unsigned  pulseId = (tp.tv_nsec >> 23) | (tp.tv_sec << 9);
      if (pulseId != _pulseId) {

	_pulseId = pulseId;

	ClockTime clockTime(sec,nsec);
	
	//  Set the L1 time with granularity of ~8.39ms (2**23 ns) / 119Hz
	Transition tr(TransitionId::L1Accept,
		      Transition::Record,
		      Sequence(Sequence::Event,
			       (Service)TransitionId::L1Accept,
			       clockTime, 0, pulseId),
		      0 );
	_control.mcast(tr);
      }

      nanosleep( &_td, 0 );

      if (_enabled)
	_task->call(this);
    }
  private:
    CollectionManager& _control;
    Task* _task;
    volatile bool  _enabled;
    unsigned _pulseId;
    timespec _td;
  };
    
}

using namespace Pds;



int main(int argc, char** argv)
{
  unsigned platform = 0;
  unsigned bldList[32];
  unsigned nbld = 0;

  int c;
  while ((c = getopt(argc, argv, "p:b:")) != -1) {
    char* endPtr;
    switch (c) {
    case 'b':
      bldList[nbld++] = strtoul(optarg, &endPtr, 0);
      break;
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = 0;
      break;
    }
  }
  if (!platform) {
    printf("usage: %s -p <platform> [-b <bld>]\n", argv[0]);
    return 0;
  }

  MyCallback callback;
  MyControl control(platform,
		    callback,
		    0);
  while(nbld--) {
    unsigned id = bldList[nbld];
    Node node(Level::Source, 0);
    node.fixup(Pds::StreamPorts::bld(id).address(),Ether());
    control.add_member(node);
  }

  control.attach();

  Task* l1task = new Task(TaskObject("l1loop"));
  MyEnable* l1action = new MyEnable(control,l1task);
  {
    timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    {
      unsigned  sec  = tp.tv_sec;
      unsigned  nsec = tp.tv_nsec&~0x7FFFFF;
      unsigned  pulseId = (tp.tv_nsec >> 23) | (tp.tv_sec << 9);
      printf("control joined at %08x/%08x pulseId %08x\n",sec,nsec,pulseId);
    }

    fprintf(stdout, "Commands: EOF=quit\n");
    while (true) {
      const int maxlen=128;
      char line[maxlen];
      char* result = fgets(line, maxlen, stdin);
      if (!result) {
        fprintf(stdout, "\nExiting\n");
        break;
      } else {
	clock_gettime(CLOCK_REALTIME, &tp);
	unsigned  sec  = tp.tv_sec;
	unsigned  nsec = tp.tv_nsec&~0x7FFFFF;
	unsigned  pulseId = (tp.tv_nsec >> 23) | (tp.tv_sec << 9);
	ClockTime clockTime(sec,nsec);
	
        int len = strlen(result)-1;
        if (len <= 0) continue;
        while (*result && *result != '\n') {
          char cmd = *result++;
          switch (cmd) {
	  case 'm':
	    {
	      control.map(Sequence(Sequence::Event,
				   (Service)TransitionId::Map,
				   clockTime, 0, pulseId));
	    }
	    break;
          case 'M':
            {
              Transition tr(TransitionId::Map,
			    Transition::Record,
			    Sequence(Sequence::Event,
				     (Service)TransitionId::Map,
				     clockTime, 0, pulseId),
			    0 );
              control.mcast(tr);
            }
            break;
          case 'c':
            {
              Transition tr(TransitionId::Configure,
			    Transition::Execute,
			    Sequence(Sequence::Event,
				     (Service)TransitionId::Configure,
				     clockTime, 0, pulseId),
			    0 );
              control.mcast(tr);
            }
            break;
          case 'C':
            {
              Transition tr(TransitionId::Configure,
			    Transition::Record,
			    Sequence(Sequence::Event,
				     (Service)TransitionId::Configure,
				     clockTime, 0, pulseId),
			    0 );
              control.mcast(tr);
            }
            break;
	  case 'e':
	    l1action->enable();
	    break;
	  case 'd':
	    l1action->disable();
	    break;
	  case 's':
	    {
	      //  Set the L1 time with granularity of ~8.39ms (2**23 ns) / 119Hz
	      Transition tr(TransitionId::L1Accept,
			    Transition::Record,
			    Sequence(Sequence::Event,
				     (Service)TransitionId::L1Accept,
				     clockTime, 0, pulseId),
			    0 );
	      control.mcast(tr);
	      //	      printf("softl1 %x/%x\n",l1key.high(),l1key.low());
	    }
          }
        }
      }  
    } 
  }

  control.detach();

  l1task->destroy();
  delete l1action;

  return 0;
}
