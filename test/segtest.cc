#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"   // SegmentOptions
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/EvrServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/service/Task.hh"
#include "pds/utility/Transition.hh"

#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPoolW.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/client/Browser.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

//#define USE_ZCP

static bool verbose = false;

namespace Pds {

  class ServerMsg {
  public:
    ServerMsg() {}
    ServerMsg(const ServerMsg& m) :
      evr    (m.evr.seq,m.evr.evr),
      ptr    (m.ptr),
      offset (m.offset),
      length (m.length)
    {}
    EvrDatagram evr;
    unsigned    ptr;
    int         offset;
    int         length;
  };

  class MySeqServer : public EvrServer, private Routine {
  public:
    MySeqServer(unsigned platform, 
		int      fd) :
      EvrServer(StreamPorts::event(platform,Level::Segment),
		DetInfo(-1UL,DetInfo::NoDetector,0,DetInfo::Evr,0),
		4),
      _task(new Task(TaskObject("segtest_evr",127))),
      _pipe(fd)
    { _task->call(this); }

    ~MySeqServer() { _task->destroy(); }

    void routine() 
    {
      ServerMsg dg;
      while(1) {
	if (fetch(reinterpret_cast<char*>(&dg),0) < 0) continue;
	dg.evr.seq = sequence();
	dg.evr.evr = count();
	dg.offset = 0;
	dg.ptr    = 0;
	::write(_pipe,&dg,sizeof(dg));
      }
    }
  private:
    Task* _task;
    int   _pipe;
  };

  class MyL1Server : public EbServer, EbCountSrv {
    enum { PayloadSize = 4*1024*1024 };
  public:
    MyL1Server(unsigned platform,
	       int size1, int size2,
	       const Src& s) : 
      _xtc(TypeId(TypeId::Any,0), s), 
      _size0   (size1),
      _dsize   (size2-size1)
    {
      for(unsigned k=0; k<PayloadSize; k++)
	_payload[k] = k&0xff;
#ifdef USE_ZCP
      int remaining = PayloadSize;
      char* p = _payload;
      while(remaining) {
	int sz = _zfragment.uinsert(p,remaining);
	_zpayload.insert(_zfragment,sz);
	remaining -= sz;
      }
#else
#endif
      ::pipe(_pipefd);
      fd(_pipefd[0]);

      _server  = new MySeqServer(platform, _pipefd[1]);
    }
    ~MyL1Server() { delete _server; }
  public:
    //  Eb interface
    void        dump    (int detail)   const {}
    bool        isValued()             const {return true;}
    const Src&  client  ()             const {return _xtc.src;}
    //  EbSegment interface
    const Xtc&  xtc   () const {return _xtc;}
    bool        more  () const {return _more;}
    unsigned    length() const {return _hdr.length;}
    unsigned    offset() const {return _hdr.offset;}
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend        (int flag = 0) {return 0;}
    int      fetch       (char* payload, int flags)
    {
      _more=false;
      ::read(_pipefd[0],&_hdr,sizeof(_hdr));
      char* p;
      int sz = _fetch(&p);
      Xtc* xtc = new (payload) Xtc(_xtc);
      memcpy(xtc->alloc(sz),p,sz);
      _hdr.length = xtc->extent;
      return _hdr.length;
    }
    int      fetch       (ZcpFragment& zf, int flags)
    {
      ::read(_pipefd[0],&_hdr,sizeof(_hdr));
      _more=true;

      if (_hdr.offset == 0) {
	char* p;
	int sz = _fetch(&p);
	Xtc xtc(_xtc);
	xtc.extent += sz;
	zf.uinsert(&xtc,sizeof(Xtc));
	_hdr.length = xtc.extent;
	ServerMsg hdr(_hdr);
	hdr.ptr    = 0;
	hdr.offset = sizeof(Xtc);
	::write(_pipefd[1],&hdr,sizeof(hdr));
	return sizeof(Xtc);
      }
      else {
	int sz  = _hdr.length - _hdr.offset;
	int len = _zpayload.remove(zf,sz);
	_zfragment.copy (zf,len);
	_zpayload.insert(_zfragment,len);
	if (len != sz) {
	  ServerMsg hdr(_hdr);
	  hdr.ptr    += len;
	  hdr.offset += len;
	  ::write(_pipefd[1],&hdr,sizeof(hdr));
	}
	return len;
      }
    }
    /***
    int      fetch       (ZcpStream& zf, int flags)
    {
      _more=false;
      ::read(_pipefd[0],&_hdr,sizeof(_hdr));
      char* p;
      int sz = _fetch(&p);
      Xtc* xtc = new (payload) Xtc(_xtc);
      memcpy(xtc->alloc(sz),p,sz);
      _hdr.length = xtc->extent;
      return _hdr.length;
    }
    ***/
  public:
    unsigned        count() const { return _hdr.evr.evr; }
  private:
    int     _fetch       (char** payload) 
    {
      int sz = (_dsize==0) ? _size0 : _size0 + ((random()%_dsize) & ~3);
      *payload = &_payload[(random()%(PayloadSize-sz)) & ~3];
      return sz;
    }
  private:
    MySeqServer* _server;
    ServerMsg    _hdr;
    int       _pipefd[2];
    Xtc       _xtc;
    bool      _more;
    int       _size0;
    int       _dsize;
    char      _payload[PayloadSize];
    char      _evrPayload[sizeof(EvrDatagram)];
    ZcpFragment _zfragment;
    ZcpStream   _zpayload;
  };

  //
  //  Sample feature extraction.
  //  Iterate through the input datagram to strip out the BLD
  //  and copy the remaining data.
  //
  class MyIterator : public XtcIterator {
  public:
    MyIterator(Datagram& dg) : _dg(dg) {}
  private:
    Datagram& _dg;
  };

  class MyFEX : public XtcIterator {
  public:
    enum { PoolSize = 32*1024*1024 };
    enum { EventSize = 2*1024*1024 };
    enum Algorithm { Input, Full, TenPercent, None, Sink, NumberOf };
  public:
    MyFEX(const Src& s) : _src(s),
			  _pool(PoolSize,EventSize), 
			  _iter(sizeof(ZcpDatagramIterator),32),
			  _outdg(0) {}
    ~MyFEX() {}

    Transition* configure(Transition* in) 
    {
      _algorithm = Algorithm(in->env().value()  % NumberOf);
      _verbose   = in->env().value() >= NumberOf;
      printf("MyFex::algorithm %d  verbose %c\n",
	     _algorithm,_verbose ? 't':'f');
      return in;
    }
    InDatagram* configure(InDatagram* in) {return in;}
    Transition* l1accept (Transition* in) {return in;}

    InDatagram* l1accept (InDatagram* input) 
    {
      if (_verbose) {
	InDatagramIterator* in_iter = input->iterator(&_iter);
	int advance;
	Browser browser(input->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }

      if (input->datagram().seq.type()   !=Sequence::Event ||
	  input->datagram().seq.service()!=TransitionId::L1Accept)
	return input;

      if (_algorithm == Input)
	return input;

      if (_algorithm == Sink)
	return 0;

      CDatagram* ndg = new (&_pool)CDatagram(input->datagram());

      {
	InDatagramIterator* in_iter = input->iterator(&_iter);
	_outdg = const_cast<Datagram*>(&ndg->datagram());
	iterate(input->datagram().xtc,in_iter);
	delete in_iter;
      }

      if (_verbose) {
	InDatagramIterator* in_iter = ndg->iterator(&_iter);
	int advance;
	Browser browser(ndg->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }

      _pool.shrink(ndg, ndg->datagram().xtc.sizeofPayload()+sizeof(Datagram));
      return ndg;
    }

    int process(const Xtc& xtc,
		InDatagramIterator* iter)
    {
      if (xtc.contains.id()==TypeId::Id_Xtc)
	return iterate(xtc,iter);
      if (xtc.src == _src) {
	_outdg->xtc.damage.increase(xtc.damage.value());
	memcpy(_outdg->xtc.alloc(sizeof(Xtc)),&xtc,sizeof(Xtc));
	int size = xtc.sizeofPayload();
	if      (_algorithm == None)       size  = 0;
	else if (_algorithm == TenPercent) size /= 10;
	iovec iov[1];
	int remaining = size;
	while( remaining ) {
	  int sz = iter->read(iov,1,size);
	  if (!sz) break;
	  memcpy(_outdg->xtc.alloc(sz),iov[0].iov_base,sz);
	  remaining -= sz;
	} 
	return size-remaining;
      }
      return 0;
    }
  private:
    Src         _src;
    Algorithm   _algorithm;
    bool        _verbose;
    RingPool    _pool;
    GenericPool _iter;
    Datagram*   _outdg;
  };

  class L1Action : public Action {
  public:
    L1Action(MyFEX& fex) : _fex(fex) {}
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* dg) { return _fex.l1accept(dg); }
  private:
    MyFEX& _fex;
  };

  class ConfigAction : public Action {
  public:
    ConfigAction(MyFEX& fex) : _fex(fex) {}
    Transition* fire(Transition* tr) { return _fex.configure(tr); }
    InDatagram* fire(InDatagram* dg) { return _fex.configure(dg); }
  private:
    MyFEX& _fex;
  };

  class BeginRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };

  class EndRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class SegTest : public EventCallback, public SegWireSettings {
  public:
    SegTest(Task*      task,
	    unsigned   platform,
	    int        s1,
	    int        s2,
	    const Src& src) :
      _task    (task),
      _platform(platform),
      _server  (new MyL1Server(platform,s1,s2,src)),
      _fex     (new MyFEX(src))
    {
      _sources.push_back(_server->client());
    }

    virtual ~SegTest()
    {
      _task->destroy();
    }
    
    // Implements SegWireSettings
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface)
    {
      wire.add_input(_server);
    }

    const std::list<Src>& sources() const { return _sources; }

  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("SegTest connected to platform 0x%x\n", 
	     _platform);

      Stream* frmk = streams.stream(StreamParams::FrameWork);
      Fsm* fsm = new Fsm;
      fsm->callback(TransitionId::L1Accept   , new L1Action    (*_fex));
      fsm->callback(TransitionId::Configure  , new ConfigAction(*_fex));
      fsm->callback(TransitionId::BeginRun   , new BeginRunAction);
      fsm->callback(TransitionId::EndRun     , new EndRunAction);
      fsm->connect(frmk->inlet());
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
    Task*       _task;
    unsigned    _platform;
    MyL1Server* _server;
    MyFEX*      _fex;
    std::list<Src> _sources;
  };
}

using namespace Pds;

void _print_help(const char* p0)
{
  printf("Usage : %s -p <platform> [-i <det_id> -a <arp_pid> -s <size_lo size_hi> -v]\n",
	 p0);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = 0;
  int      size1    = 1024;
  int      size2    = 1024;
  unsigned detid = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:p:s:vh")) != EOF ) {
    switch(c) {
    case 'i':
      detid  = strtoul(optarg, NULL, 0);
      break;
    case 's':
      if (sscanf(optarg,"%d,%d",&size1,&size2)==1)
	size2 = size1;
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'v':
      verbose = true;
      break;
    case '?':
      printf("Unrecognized option %c\n",c);
    case 'h':
      _print_help(argv[0]);
      exit(1);
    }
  }

  if (!platform) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,platform);
  SegTest* segtest = new SegTest(task, platform, 
				 size1, size2, 
				 DetInfo(node.pid(),DetInfo::AmoGasdet,
					 detid,DetInfo::Opal1000,0));
  SegmentLevel* segment = new SegmentLevel(platform, 
					   *segtest,
					   *segtest,
					   (Arp*)0); 
  
  if (segment->attach())
    task->mainLoop();

  segment->detach();

  return 0;
}
