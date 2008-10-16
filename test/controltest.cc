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

//   class MyState {
//   public:
//     enum Id { Standby, Mapped, Configured, Running, Enabled, Paused, Unknown };
//     MyState( Id id ) : _id(id) {}
//     TransitionId::Value next(Id tgt) {
//       TransitionId::Value v;
//       if      (tgt==Paused) v=TransitionId::Pause;
//       else if (_id==Paused) v=TransitionId::Resume;
//       else if (tgt > _id) {
// 	switch(_id) {
// 	case Standby: v=TransitionId::Map; break; 
// 	case Mapped : v=TransitionId::Configure; break; 
// 	case Configured: v=TransitionId::BeginRun; break; 
// 	case Running: v=TransitionId::Enable; break; 
// 	default: v=TransitionId::Unknown; break;
// 	}
//       }
//       else {
// 	switch(_id) {
// 	case Mapped : v=TransitionId::Unmap; break; 
// 	case Configured: v=TransitionId::Unconfigure; break; 
// 	case Running: v=TransitionId::EndRun; break; 
// 	case Enabled: v=TransitionId::Disable; break; 
// 	default: v=TransitionId::Unknown; break;
// 	}
//       }
//       return v;
//     }
//   private:
//     Id _id;
//   };

  //  Add partition management to control level
  class MyControl : public ControlLevel {
  public:
    MyControl(unsigned platform, ControlCallback& cb, Arp* arp) :
      ControlLevel(platform, cb, arp),
      _reporters ("Reporters",-1UL),
      _members   ("Members",-1UL),
      _partition ("Partition",-1UL),
      _transition("Transition",-1UL)
    {
    }
    ~MyControl() {}

    void add_reporter(const Node& n)
    {
      _reporters.add(n);
    }

    void send(Message& msg) {
    }

    void map(const Sequence& seq)
    {
      //  Equate the partition id with the platform id until we have a master
      _partition = Allocate("partition",header().platform(),seq);
      unsigned nnodes = _reporters.nnodes();
      while(nnodes--)
	_partition.add( *_reporters.node(nnodes) );
      nnodes = _members.nnodes();
      while(nnodes--)
	_partition.add( *_members.node(nnodes) );
      mcast(_partition);
    }

    void unmap()
    {
      //  Equate the partition id with the platform id until we have a master
      _partition = Allocate("partition",header().platform());
      Kill kill(header());
      mcast(kill);
    }

    void ping()
    {
      _members = Allocate("Members",header().platform());
      Message ping(Message::Ping);
      mcast(ping);
    }

    void message(const Node& hdr, const Message& msg)
    {
      switch(msg.type()) {
      case Message::Ping:
      case Message::Join:
	if (!_isallocated) {
	  unsigned k;
	  for(k=0; k<_members.nnodes(); k++)
	    if (*_members.node(k)==hdr) break;
	  if (k==_members.nnodes()) {
	    printf("Found node %x/%d/%d\n",hdr.ip(),hdr.level(),hdr.pid());
	    _members.add(hdr);
	  }
	}
	break;
      case Message::Transition:
	//	printf("Transition adds %x/%d/%d\n",hdr.ip(),hdr.level(),hdr.pid());
	//	_transition.add(hdr);
	break;
      default:
	break;
      }
      PartitionMember::message(hdr,msg);
    }
  private:
    Allocate _reporters;
    Allocate _members;
    Allocate _partition;
    Allocate _transition;
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

      nanosleep( &_td, 0 );

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

      if (_enabled)
	_task->call(this);
    }
    ~MyEnable() {
      _task->destroy();
    }
  private:
    CollectionManager& _control;
    Task* _task;
    volatile bool  _enabled;
    unsigned _pulseId;
    timespec _td;
  };
    
  class TransitionAction : public Appliance {
  public:
    TransitionAction(CollectionManager& control) :
      _action(new MyEnable(control,new Task(TaskObject("l1loop")))) 
    {}
    ~TransitionAction() { delete _action; }
    Transition* transitions(Transition* i) { 
      if      (i->id() == TransitionId::Enable)  _action->enable();
      else if (i->id() == TransitionId::Disable) _action->disable();
      return 0; 
    }
    InDatagram* occurrences(InDatagram* i) { return i; } 
    InDatagram* events     (InDatagram* i) { return i; }
  private:
    MyEnable* _action;
  };

  class MyCallback : public ControlCallback {
  public:
    void outlet(CollectionManager& o) { _outlet=&o; }
    void allocated(SetOfStreams& streams) {
      //  By default, there are no external clients for this stream
      Stream& frmk = *streams.stream(StreamParams::FrameWork);
      frmk.outlet()->sink(Sequence::Event, (1<<Sequence::NumberOfServices)-1);
      (new Decoder(Level::Control))->connect(frmk.inlet());
      (new TransitionAction(*_outlet))->connect(frmk.inlet());
      printf("Partition allocated\n");
    }
    void failed(Reason reason) {
      printf("Partition failed to allocate: reason %d\n", reason);
    }
    void dissolved(const Node& node) {
      printf("Partition dissolved by uid %d pid %d ip %x\n",
	     node.uid(), node.pid(), node.ip());
    }
  private:
    CollectionManager* _outlet;
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
  callback.outlet(control);

  while(nbld--) {
    unsigned id = bldList[nbld];
    Node node(Level::Source, 0);
    node.fixup(Pds::StreamPorts::bld(id).address(),Ether());
    control.add_reporter(node);
  }

  control.attach();

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
// 	  MyState::Id target;
// 	  switch(cmd) {
// 	  case 's': target = MyState::Standby; break;
// 	  case 'm': target = MyState::Mapped; break;
// 	  case 'c': target = MyState::Configured; break;
// 	  case 'r': target = MyState::Running; break;
// 	  case 'e': target = MyState::Enabled; break;
// 	  case 'p': target = MyState::Paused; break;
// 	  default:  target = MyState::Unknown; break;
// 	  }
// 	  control.target(target);
// 	}
	  switch(cmd) {
	  case 'P':
	    control.ping();
	    break;
	  case 'm':
	    control.map(Sequence(Sequence::Event,
				 (Service)TransitionId::Map,
				 clockTime, 0, pulseId));
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
	  case 'u':
	    control.unmap();
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
            {
              Transition tr(TransitionId::Enable,
			    Transition::Execute,
			    Sequence(Sequence::Event,
				     (Service)TransitionId::Configure,
				     clockTime, 0, pulseId),
			    0 );
              control.mcast(tr);
            }
	    break;
	  case 'd':
            {
              Transition tr(TransitionId::Disable,
			    Transition::Execute,
			    Sequence(Sequence::Event,
				     (Service)TransitionId::Configure,
				     clockTime, 0, pulseId),
			    0 );
              control.mcast(tr);
            }
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

  return 0;
}
