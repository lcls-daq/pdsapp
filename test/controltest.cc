#include "pds/collection/CollectionManager.hh"
#include "pds/utility/Transition.hh"
#include "pds/collection/Route.hh"
#include "pds/collection/CollectionPorts.hh"

#include "pds/service/Semaphore.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/management/ControlLevel.hh"
#include "pds/client/Decoder.hh"

#include "pds/service/Task.hh"
#include "pds/service/Semaphore.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>

static int l1rate=0;
static bool verbose;

namespace Pds {

  class NodeL : public LinkedList<NodeL> {
  public:
    NodeL(const Node& n) : node(n) {}
    Node node;
    PoolDeclare;
  };

  void clearNodeList(LinkedList<NodeL>& list) {
    NodeL* n;
    while( (n=list.forward()) != list.empty() )
      delete n->disconnect();    
  }

  //  Add partition management to control level
  class MyControl : public ControlLevel {
  public:
    MyControl(const char* partition,
	      const char* dbpath,
	      unsigned platform, 
	      ControlCallback& cb, 
	      Arp* arp) :
      ControlLevel(platform, cb, arp),
      _description(partition),
      _dbpath     (dbpath),
      _nodePool   (sizeof(NodeL), 128),
      _trnsPool   (sizeof(Allocate),2),
      _trnsSem    (Semaphore::EMPTY),
      _pending    (0),
      _nnodes     (0)
    {
    }
    ~MyControl() {}

    void add_reporter(const Node& n)
    {
      _reporters.insert(new(&_nodePool) NodeL(n));
    }

    void execute(Transition& tr) {
      clearNodeList(_transition);

      NodeL* n=_partition.forward();
      while( n != _partition.empty() ) {
	_transition.insert(new(&_nodePool) NodeL(n->node));
	n = n->forward();
      }

      mcast(tr);

      _trnsSem.take();  // block until transition is complete
    }

    void map(const Sequence& seq, unsigned mask)
    {
      if (!mask) mask=-1UL;
      printf("Mapping nodes 0x%x\n",mask);

      //  Equate the partition id with the platform id until we have a master
      Allocation partition(_description,_dbpath,partitionid());
      clearNodeList(_partition);

      NodeL* n = _reporters.forward();
      while( n != _reporters.empty()) {
	partition.add( n->node );
	n = n->forward();
      }
      n = _platform.forward();
      unsigned inode=0;
      unsigned imask=0;
      while( n != _platform.empty() ) {
	if ((1<<inode)&mask) {
	  imask |= (1<<inode);
	  _partition.insert( new(&_nodePool) NodeL( n->node ) );
	  partition.add( n->node );
	}
	n = n->forward();
	inode++;
      }
      Allocate alloc(partition);
      execute(alloc);
      
      printf("Successfully mapped nodes 0x%x\n",imask);
    }

    void unmap()
    {
      Kill kill(header());
      execute(kill);
      clearNodeList(_partition);
    }

    void ping()
    {
      clearNodeList(_platform);
      _nnodes=0;
      Message ping(Message::Ping);
      mcast(ping);
    }

    void message(const Node& hdr, const Message& msg)
    {
      switch(msg.type()) {
      case Message::Ping:
      case Message::Join:
	if (!_isallocated) {
	  printf("Node [0x%08x] %x/%d/%d\n",(1<<_nnodes),hdr.ip(),hdr.level(),hdr.pid());
	  _platform.insert(new(&_nodePool)NodeL(hdr));
	  _nnodes++;
	}
	break;
      case Message::Transition:
	{
	  const Transition& tr = reinterpret_cast<const Transition&>(msg);
	  if (tr.phase() == Transition::Execute) {
	    if (hdr == header()) {
	      _pending = new(&_trnsPool)Transition(tr);
	    }

	    NodeL* n = _transition.forward();
	    while( n != _transition.empty() ) {
	      if ( n->node == hdr ) {
		delete n->disconnect();
		if (_transition.forward() == _transition.empty()) {
		  PartitionMember::message(header(),*_pending);
		  if (_pending) {
		    delete _pending;
		    _pending = 0;
		    _trnsSem.give();
		  }
		  else
		    printf("--- No pending transition! ---\n");
		}
	      }
	      n = n->forward();
	    }
	    return;
	  }
	}
	break;
      default:
	break;
      }
      ControlLevel::message(hdr,msg);
    }

  private:
    const char*       _description;
    const char*       _dbpath;
    GenericPool       _nodePool;
    GenericPool       _trnsPool;
    Semaphore         _trnsSem;
    LinkedList<NodeL> _reporters;
    LinkedList<NodeL> _platform;
    LinkedList<NodeL> _partition;
    LinkedList<NodeL> _transition;
    Transition*       _pending;
    unsigned          _nnodes;
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
      _td.tv_nsec = (1<<30)/l1rate;
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
			       TransitionId::L1Accept,
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
      return i; 
    }
    InDatagram* occurrences(InDatagram* i) { return i; } 
    InDatagram* events     (InDatagram* i) { return i; }
  private:
    MyEnable* _action;
  };

  class ControlAction : public Appliance {
  public:
    ControlAction(CollectionManager& control) :
      _control(control)
    {}
    ~ControlAction() {}
  public:
    Transition* transitions(Transition* i) { 
      if (i->phase() == Transition::Execute) {
	if (i->id()==TransitionId::Disable) {
	  //
	  //  The disable transition often splits the last L1Accept.
	  //  There is no way to know when the last L1A has passed through
	  //  all levels, so wait some reasonable time.
	  //
	  timespec tv;
	  tv.tv_sec = 0; tv.tv_nsec = 50000000;
	  nanosleep(&tv, 0);
	}
	Transition tr(i->id(),
		      Transition::Record,
		      Sequence(Sequence::Event,
			       i->id(),
			       i->sequence().clock(), 
			       i->sequence().low(),
			       i->sequence().high()),
		      i->env() );
	_control.mcast(tr);
      }
      return 0;
    }
    InDatagram* occurrences(InDatagram* i) { return 0; } 
    InDatagram* events     (InDatagram* i) { return 0; }
  private:
    CollectionManager& _control;
  };

  class MyCallback : public ControlCallback {
  public:
    void outlet(CollectionManager& o) { _outlet=&o; }
    void allocated(SetOfStreams& streams) {
      //  By default, there are no external clients for this stream
      Stream& frmk = *streams.stream(StreamParams::FrameWork);
      //      frmk.outlet()->sink(Sequence::Event, (1<<Sequence::NumberOfServices)-1);
      (new ControlAction(*_outlet))   ->connect(frmk.inlet());
      if (l1rate) 
	(new TransitionAction(*_outlet))->connect(frmk.inlet());
      (new Decoder(Level::Control))   ->connect(frmk.inlet());
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
  const char* partition = "partition";
  const char* dbpath    = "none";
  verbose = false;

  int c;
  while ((c = getopt(argc, argv, "p:b:r:P:D:v")) != -1) {
    char* endPtr;
    switch (c) {
    case 'b':
      bldList[nbld++] = strtoul(optarg, &endPtr, 0);
      break;
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = 0;
      break;
    case 'r':
      l1rate = strtoul(optarg, &endPtr, 0);
      if (l1rate == 0) l1rate = 200;
      break;
    case 'P':
      partition = optarg;
      break;
    case 'D':
      dbpath = optarg;
      break;
    case 'v':
      verbose = true;
      break;
    }
  }
  if (!platform) {
    printf("usage: %s -p <platform> [-b <bld> -r <l1rate> -P <partition_description> -D <db name>]\n", argv[0]);
    return 0;
  }

  MyCallback callback;
  MyControl control(partition,
		    dbpath,
		    platform,
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
      printf("Commands (P)ing, (m)ap, (c)onfigure, begin(r)un, (e)nable,\n"
	     "         (d)isable, end(R)un, un(C)onfigure, un(M)ap: \n");
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
	  unsigned env = strtoul(result,&result,16);
	  if      (cmd=='P') control.ping();
	  else if (cmd=='m') control.map(Sequence(Sequence::Event,
						  TransitionId::Map,
						  clockTime, 0, pulseId),
					 env);
	  else if (cmd=='M') control.unmap();
	  else {
	    TransitionId::Value id;
	    switch(cmd) {
	    case 'c': id = TransitionId::Configure; break;
	    case 'C': id = TransitionId::Unconfigure; break;
	    case 'r': id = TransitionId::BeginRun; break;
	    case 'R': id = TransitionId::EndRun; break;
	    case 'e': id = TransitionId::Enable; break;
	    case 'd': id = TransitionId::Disable; break;
	    case 's':
	      {
		id = TransitionId::L1Accept;
		Transition tr(id,
			      Transition::Record,
			      Sequence(Sequence::Event,
				       id,
				       clockTime, 0, pulseId),
			      env);
		control.mcast(tr);
		continue;
	      }
	    default:
	      continue;
	    }
	    Transition tr(id,
			  Transition::Execute,
			  Sequence(Sequence::Event,
				   id,
				   clockTime, 0, pulseId),
			  env );
	    control.execute(tr);
	  }
        }
      }  
    } 
  }

  control.detach();

  return 0;
}
