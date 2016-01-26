#include "pdsapp/monobs/MonComm.hh"

#include "pdsapp/tools/PnccdShuffle.hh"
#include "pdsapp/tools/CspadShuffle.hh"

#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/service/GenericPool.hh"

#include "pds/utility/InletWireServer.hh"
#include "pds/utility/WiredStreams.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/xtc/CDatagram.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/app/XtcMonitorServer.hh"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>

static inline bool rantest(double f)
{
  return rand() < int(double(RAND_MAX)*f);
}

using namespace Pds;

class DynamicObserver : public ObserverLevel,
                        public MonEventControl {
public:
  DynamicObserver(unsigned       platform,
		  const char*    partition,
		  unsigned       node_mask,
		  EventCallback& display,
		  int            slowReadout,
		  unsigned       sizeOfBuffers) :
    ObserverLevel(platform, partition, node_mask, display, slowReadout, sizeOfBuffers),
    _mask        (node_mask),
    _nodes       (0),
    _changed     (true)
  {}
public:
  void allocated(const Allocation& alloc) 
  {
    ObserverLevel::allocated(alloc);
    _alloc=alloc; 
    apply();
    _changed = true;
  }
  void dissolved() 
  {
    ObserverLevel::dissolved();
    //  clear the partition (in case we apply() while unmapped)
    _alloc=Allocation(_alloc.partition(),
		      _alloc.dbpath(),
		      _alloc.partitionid());
    _nodes=0;
    _changed = true;
  }
  void set_mask(unsigned mask)
  {
    _mask = mask;
    apply();  // this may be risky
  }
  void apply()
  {
    InletWireServer* inlet = static_cast<InletWireServer*>(streams()->wire(StreamParams::FrameWork));
    unsigned partition  = _alloc.partitionid();
    unsigned nnodes     = _alloc.nnodes();
    unsigned segmentid  = 0;
    _nodes = 0;
    for (unsigned n=0; n<nnodes; n++) {
      const Node& node = *_alloc.node(n);
      if (node.level() == Level::Segment) {
	NetDgServer* srv = static_cast<NetDgServer*>(inlet->server(segmentid));
	srv->server().resign();
	for(unsigned mask=_mask,index=0; mask!=0; mask>>=1,index++) {
	  if (mask&1) {
	    Ins mcastIns(StreamPorts::event(partition,
					    Level::Event,
					    index,
					    segmentid).address());
	    srv->server().join(mcastIns, Ins(header().ip()));
	  }
	}
	Ins bcastIns = StreamPorts::bcast(partition, Level::Event);
	srv->server().join(bcastIns, Ins(header().ip()));
	segmentid++;
      } // if (node.level() == Level::Segment) {
      else if (node.level() == Level::Event)
	_nodes++;
    } // for (unsigned n=0; n<nnodes; n++) {
  }
public:
  bool     changed() { bool v(_changed); _changed=false; return v; }
  unsigned nodes() const { return _nodes; }
  unsigned mask() const { return _mask; }
  unsigned short port() const { return MonShmComm::ServerPort - header().platform(); }
private:
  Allocation _alloc;
  unsigned   _mask;
  unsigned   _nodes;
  bool       _changed;
};

class LiveMonitorServer : public Appliance,
                          public XtcMonitorServer {
public:
  LiveMonitorServer(const char* tag,
                    unsigned sizeofBuffers,
                    int numberofEvBuffers,
                    unsigned numberofEvQueues,
		    double dropProb=0) :
    XtcMonitorServer(tag, sizeofBuffers, numberofEvBuffers, numberofEvQueues),
    _upool          (new GenericPool(sizeof(CDatagram),1)),
    _dropProb       (dropProb)
  {
    _init();
  }
  ~LiveMonitorServer()
  {
    delete _upool;
  }

public:
  Transition* transitions(Transition* tr)
  {
    printf("LiveMonitorServer tr %s\n",TransitionId::name(tr->id()));
    if (tr->id() == TransitionId::Unmap) {
      //
      //  Generate an Unmap datagram
      //
      CDatagram* unmap = new (_upool) CDatagram(Datagram(*tr, TypeId(TypeId::Id_Xtc,0),ProcInfo(Level::Observer,getpid(),0)));
      Dgram& dgrm = reinterpret_cast<Dgram&>(unmap->datagram());
      XtcMonitorServer::events(&dgrm);
      delete unmap;
      
      //      _pop_transition();
    }
    return tr;
  }

  InDatagram* occurrences(InDatagram* dg) { return dg; }

  InDatagram* events     (InDatagram* dg)
  {
    Dgram& dgrm = reinterpret_cast<Dgram&>(dg->datagram());
    if (dgrm.seq.service()!=TransitionId::L1Accept)
      printf("LiveMonitorServer dg %s\n",TransitionId::name(dgrm.seq.service()));
    return
      (XtcMonitorServer::events(&dgrm) == XtcMonitorServer::Deferred) ?
      (InDatagram*)DontDelete : dg;
  }

private:
  void _copyDatagram(Dgram* dg, char* b)
  {
    Datagram& dgrm = *reinterpret_cast<Datagram*>(dg);

    PnccdShuffle::shuffle(dgrm);
    CspadShuffle::shuffle(reinterpret_cast<Dgram&>(dgrm));

    //  write the datagram
    memcpy(b, &dgrm, sizeof(Datagram));
    //  write the payload
    memcpy(b+sizeof(Datagram), dgrm.xtc.payload(), dgrm.xtc.sizeofPayload());
  }

  void _deleteDatagram(Dgram* dg)
  {
    Datagram& dgrm = *reinterpret_cast<Datagram*>(dg);
    InDatagram* indg = static_cast<InDatagram*>(&dgrm);
    delete indg;
  }
private:
  Pool* _upool;
  double _dropProb;
};

#include "pds/utility/EbBase.hh"

class MyEbDump : public Appliance {
public:
  MyEbDump(InletWire* eb) : _eb(static_cast<EbBase*>(eb)) {}
public:
  Transition* transitions(Transition* in) { return in; }
  InDatagram* occurrences(InDatagram* in) { return in; }
  InDatagram* events     (InDatagram* in) {
    const Datagram& dg = in->datagram();
    if (dg.seq.service()==TransitionId::EndRun)
      _eb->dump(1);
    return in;
  }
private:
  EbBase* _eb;
};

class Stats : public Appliance,
              public MonEventStats {
public:
  Stats() : _events(0), _dmg(0), _changed(true) {}
  ~Stats() {}
public:
  Transition* transitions(Transition* tr) { return tr; }
  InDatagram* events(InDatagram* dg) {
    if (dg->datagram().seq.service()==TransitionId::L1Accept) {
      _events++;
      if (dg->datagram().xtc.damage.value()&(1<<Damage::ContainsIncomplete))
	_dmg++;
      _changed=true;
    }
    return dg;
  }
  bool     changed() { bool v(_changed); _changed=false; return v; }
  unsigned events() const { return _events; }
  unsigned dmg   () const { return _dmg; }
private:
  unsigned _events;
  unsigned _dmg;
  bool     _changed;
};

class MyCallback : public EventCallback {
public:
  MyCallback(Task* task, Appliance* app) :
    _task(task),
    _appliances(app)
  {
  }
  ~MyCallback() {}

  void attached (SetOfStreams& streams)
  {
    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _appliances->connect(frmk->inlet());
    (new MyEbDump(streams.wire()))->connect(frmk->inlet());
  }
  void failed   (Reason reason)   {}
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  Appliance*  _appliances;
};

void usage(char* progname) {
  printf("Usage: %s -p <platform> -P <partition> -i <node mask> -n <numb shm buffers> -s <shm buffer size> [options]\n"
	 "Options: -q <# event queues>  (number of queues to hold for clients)\n"
	 "         -t <tag name>        (name of shared memory)\n"
	 "         -d                   (distribute events)\n"
	 "         -c                   (allow remote control of node mask)\n"
	 "         -g <max groups>      (number of event nodes receiving data)\n"
	 "         -T <drop prob>       (test mode - drop transitions)\n"
	 "         -h\n", progname);
}

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = unsigned(-1UL);
  unsigned platform=NO_PLATFORM;
  const char* partition = 0;
  const char* tag = 0;
  int numberOfBuffers = 0;
  unsigned sizeOfBuffers = 0;
  unsigned nevqueues = 1;
  unsigned node =  0xffff;
  unsigned nodes = 6;
  bool ldist = false;
  bool lcomm = false;
  Appliance* uapps = 0;
  int slowReadout = 0;
  double dropProb=0;

  printf("Command line: ");
  for(int i=0; i<argc; i++)
    printf("%s ",argv[i]);
  printf("\n\n");

  int c;
  while ((c = getopt(argc, argv, "p:i:g:n:P:s:q:L:w:t:dcT:h")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = NO_PLATFORM;
      break;
    case 'i':
      node = strtoul(optarg, &endPtr, 0);
      break;
    case 'g':
      nodes = strtoul(optarg, &endPtr, 0);
      break;
    case 'n':
      sscanf(optarg, "%d", &numberOfBuffers);
      break;
    case 'P':
      partition = optarg;
      break;
    case 't':
      tag = optarg;
      break;
    case 'q':
      nevqueues = strtoul(optarg, NULL, 0);
      break;
    case 's':
      sizeOfBuffers = (unsigned) strtoul(optarg, NULL, 0);
      break;
    case 'd':
      ldist = true;
      break;
    case 'c':
      lcomm = true;
      break;
    case 'w':
      slowReadout = strtoul(optarg, NULL, 0);
      break;
    case 'T':
      dropProb = strtod(optarg, NULL);
      break;
    case 'L':
      { for(const char* p = strtok(optarg,","); p!=NULL; p=strtok(NULL,",")) {
    printf("dlopen %s\n",p);

    void* handle = dlopen(p, RTLD_LAZY);
    if (!handle) {
      printf("dlopen failed : %s\n",dlerror());
      break;
    }

    // reset errors
    const char* dlsym_error;
    dlerror();

    // load the symbols
    create_app* c_user = (create_app*) dlsym(handle, "create");
    if ((dlsym_error = dlerror())) {
      fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
      break;
    }
    if (uapps)
      c_user()->connect(uapps);
    else
      uapps = c_user();
  }
  break;
      }
    case 'h':
      // help
      usage(argv[0]);
      return 0;
      break;
    default:
      printf("Unrecogized parameter\n");
      usage(argv[0]);
      break;
    }
  }

  if (!numberOfBuffers || !sizeOfBuffers || platform == NO_PLATFORM || !partition || node == 0xffff) {
    fprintf(stderr, "Missing parameters!\n");
    usage(argv[0]);
    return 1;
  }

  if (numberOfBuffers<8) numberOfBuffers=8;

  if (!tag) tag=partition;

  printf("\nPartition Tag:%s\n", tag);

  Stats* stats = new Stats;

  LiveMonitorServer* apps = new LiveMonitorServer(tag, 
						  sizeOfBuffers, 
						  numberOfBuffers, 
						  nevqueues,
						  dropProb);
  apps->distribute(ldist);

  apps->connect(stats);

  if (uapps)
    uapps->connect(stats);

  if (apps) {
    Task* task = new Task(Task::MakeThisATask);
    MyCallback* display = new MyCallback(task,
					 stats);

    DynamicObserver* event = 
      new DynamicObserver(platform,
			  partition,
			  node,
			  *display,
			  slowReadout,
			  sizeOfBuffers);

    if (event->attach()) {

      if (lcomm)
        new MonComm(*event, *stats, nodes);

      task->mainLoop();
      event->detach();
      delete stats;
      delete event;
      delete display;
    }
    else {
      printf("Observer failed to attach to platform\n");
      delete stats;
      delete event;
    }
    delete apps;
    apps = 0 ;
  } else {
    printf("%s: Error creating LiveMonitorServer\n", __FUNCTION__);
  }
  return 0;
}

