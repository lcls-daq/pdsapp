#include "pds/management/ObserverStreams.hh"
#include "pds/management/CollectionObserver.hh"
#include "pds/management/EventCallback.hh"
#include "pds/service/Task.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/OpenOutlet.hh"
#include "pds/utility/OutletWire.hh"
#include "pds/utility/InletWire.hh"
#include "pds/client/Decoder.hh"
#include "pds/xtc/CDatagram.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"

static const int netbufdepth=8;
static const int MaxSize = 1024*1024;
static Pds::Allocation* _allocation;
static Pds::ProcInfo _srcobs(Pds::Level::Observer,0,0);

namespace Pds {
  class MyServer : public BldServer {
  public:
    MyServer(const Ins& ins,
	     const Src& src,
	     unsigned   maxbuf) : BldServer(ins,src,maxbuf) {}
    ~MyServer() {}
  public:
    bool isValued() const { return true; }
  };

  class MyLevel : public CollectionObserver {
  public:
    MyLevel(unsigned       platform,
	    const char*    partition,
	    uint64_t       mask,
	    EventCallback& callback) :
      CollectionObserver(platform, partition),
      _mask    (mask),
      _callback(callback),
      _streams (0) {}
    ~MyLevel() {}
  public:
    bool attach();
    void detach();
    void allocated(const Allocation& alloc);
    void dissolved();
  private:
    void     post      (const Transition&);
    void     post      (const InDatagram&);
  private:
    uint64_t       _mask;
    EventCallback& _callback;         // object to notify
    ObserverStreams * _streams;          // appliance streams
    OutletWire*    _outlets[StreamParams::NumberOfStreams];
  };

  class NodeStats {
  public:
    NodeStats() { counts=0; mask=0; memset(damage,0, sizeof(damage)); }
    ~NodeStats() {}
  public:
    void dump() const {
      if (counts) {
	printf("%20s : dmg 0x%08x  events %d\n",
	       BldInfo::name(BldInfo(0,BldInfo::Type(id))),
	       mask, counts);
	for(int i=0; i<32; i++)
	  if (damage[i])
	    printf("%*c%8d : %d\n",22,' ',i,damage[i]);
      }
    }
  public:
    unsigned id;
    unsigned counts;
    unsigned mask;
    unsigned damage[32];
  };

  static const unsigned NO_PULSE = (unsigned)-1;
  class MyHist {
  public:
    MyHist() : _last(NO_PULSE), _sum(0) { memset(_counts,0,sizeof(_counts)); }
    ~MyHist() {}
  public:
    void accumulate(unsigned pid) {
      if (pid >= MaxPulse)
	printf("Bad Pulse ID 0x%x\n",pid);
      else {
	if (_last != NO_PULSE) {
	  int index = pid-_last;
	  if (index < 0) 
	    index += MaxPulse;
	  index += ZeroBin;
	  if (index<0) 
	    index=0;
	  else if (index>=NumBins) 
	    index=NumBins-1;
	  _counts[index]++;
	}
	_sum++;
	_last = pid;
      }
    }
    void dump() const {
      if (_sum)
	for(unsigned i=0; i<NumBins; i++)
	  printf("%08d%c",_counts[i],(i%8)==7?'\n':' ');
    }
  private:
    enum { NumBins=32 };
    enum { ZeroBin=13 };
    enum { MaxPulse=Pds::TimeStamp::MaxFiducials };
    unsigned _counts[NumBins];
    unsigned _last;
    unsigned _sum;
  };

  class MyStats : public Appliance, public XtcIterator {
  public:
    MyStats(int n) : _n(n > 0 ? n : 1), _count(_n), _pool(sizeof(ZcpDatagramIterator),1) 
    {
      for(int i=0; i<BldInfo::NumberOf; i++)
	_stats[i].id = i;
    }
    ~MyStats() {}
  public:
    int process(const Xtc& xtc, InDatagramIterator* iter) {
      int advance = 0;
      if (xtc.contains.id() == TypeId::Id_Xtc)
	advance += iterate(xtc,iter);
      else if (xtc.src.level() == Level::Reporter) {
	const BldInfo& info = static_cast<const BldInfo&>(xtc.src);
	_stats[info.type()].counts++;
	_hists[info.type()].accumulate(_seq.stamp().fiducials());
	unsigned dmg = xtc.damage.value();
	_stats[info.type()].mask |= dmg;
	for(int i=0; i<32; i++) {
	  if (dmg & (0x1<<i))
	    _stats[info.type()].damage[i]++;
	}
#ifdef DUMP
	printf("%s ",BldInfo::name(info));
	const unsigned* d = reinterpret_cast<const unsigned*>(&xtc);
	const unsigned* e = d + (xtc.extent>>2);
	for(unsigned i=0; d<e; i++,d++)
	  printf(":%08x",*d);
	printf("\n");
#endif
      }	
      return advance;
    }

    Transition* transitions(Transition* in) { return in; }

    InDatagram* events     (InDatagram* in) {
      
      const Sequence& seq = in->datagram().seq;
      if (seq.stamp() <= _seq.stamp()) 
	printf("%x follows %x\n",seq.stamp().fiducials(),_seq.stamp().fiducials());

      _seq = seq;

#ifdef DUMP
      printf("-- seq %05x --\n",seq.stamp().fiducials());
#endif
      InDatagramIterator* iter = in->iterator(&_pool);
      process(in->datagram().xtc, iter);
      delete iter;

      if (--_count == 0) {
	printf("== seq %05x ==\n", seq.stamp().fiducials());
	for(int i=0; i<BldInfo::NumberOf; i++) {
	  _stats[i].dump();
	  _hists[i].dump();
	}
	_count = _n;
      }
      return in;
    }
  private:
    int _n, _count;
    NodeStats _stats[BldInfo::NumberOf];
    MyHist    _hists[BldInfo::NumberOf];
    GenericPool _pool;
    Sequence    _seq;
  };

  class MyCallback : public EventCallback {
  public:
    MyCallback(Task* task, bool decode) :
      _task   (task),
      _decode (decode)
    {
    }
    ~MyCallback() {}
    
    void attached (SetOfStreams& streams) 
    {
      Stream* frmk = streams.stream(StreamParams::FrameWork);
      if (_decode)
	(new Decoder(Level::Event))->connect(frmk->inlet());
      (new MyStats(100))->connect(frmk->inlet());
    }
    void failed   (Reason reason)   { _task->destroy(); delete this; }
    void dissolved(const Node& who) { _task->destroy(); delete this; }
  private:
    Task*       _task;
    bool        _decode;
  };
};

using namespace Pds;

bool MyLevel::attach()
{
  start();
  if (!connect()) {
    _callback.failed(EventCallback::PlatformUnavailable);
    return false;
  }

  _streams = new ObserverStreams(*this);
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _streams->stream(s)->outlet()->wire();
    _outlets[s] = new OpenOutlet(*_streams->stream(s)->outlet());
  }
  _streams->connect();
  
  Allocation alloc("private partition",
		   "no database",
		   0);
  { Node n(Level::Observer, 0);
    n.fixup(header().ip(),Ether());
    alloc.add(n); }
  for(int i=0; i<BldInfo::NumberOf; i++) {
    if (_mask & (1ULL<<i)) {
      Node n(Level::Reporter, 0);
      n.fixup(StreamPorts::bld(i).address(),Ether());
      alloc.add(n);
    }
  }
  _allocation = &alloc;

  _srcobs = header().procInfo();

  allocated(*_allocation);

  _callback.attached(*_streams);
  
  return true;
}


void MyLevel::allocated(const Allocation& alloc)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);

  // setup BLD servers
  unsigned nnodes     = alloc.nnodes();
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (node.level()==Level::Reporter) {
      Ins ins(node.ip(), StreamPorts::bld(0).portId());

      MyServer* srv = new MyServer(ins,
				   node.procInfo(),
				   netbufdepth*MaxSize);
      wire->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));

      printf("MyLevel::allocated assign fragment %d  %x/%d\n",
	     srv->id(),mcastIns.address(),srv->server().portId());
    }
  }
}

void MyLevel::post     (const Transition& tr) 
{ _streams->wire(StreamParams::FrameWork)->post(tr); }

void MyLevel::post     (const InDatagram& tr) 
{ _streams->wire(StreamParams::FrameWork)->post(tr); }

void MyLevel::dissolved()
{
  _streams->disconnect();
  _streams->connect();
}

void MyLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
  }
  cancel();
}


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

void usage(char* p)
{
  printf("Usage: %s -p <platform> -P <partition>\n", p);
}

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = (unsigned)-1;
  unsigned platform=NO_PLATFORM;
  const char* partition = 0;
  uint64_t mask=(1ULL<<BldInfo::NumberOf)-1;
  bool decode=false;

  int c;
  while ((c = getopt(argc, argv, "p:P:m:e")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = NO_PLATFORM;
      break;
    case 'P':
      partition = optarg;
      break;
    case 'm':
      mask = strtoull(optarg, &endPtr, 0);
      break;
    case 'e':
      decode=true;
      break;
    default:
      break;
    }
  }

  if (platform == NO_PLATFORM || !partition) {
    fprintf(stderr, "Missing parameters!\n");
    usage(argv[0]);
    return 1;
  }

  Task* task = new Task(Task::MakeThisATask);

  MyCallback* display = new MyCallback(task, decode);

  MyLevel* event = new MyLevel(platform,
			       partition,
			       mask,
			       *display);

  if (event->attach())
    task->mainLoop();
  else
    printf("Observer failed to attach to platform\n");

  event->detach();

  delete event;
  return 0;
}

