#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"   // SegmentOptions
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/EvrServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/ToEb.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/Xtc.hh"
#include "pds/utility/Transition.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/client/Browser.hh"
#include "pds/client/Decoder.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPoolW.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;
static unsigned detid = 0;

namespace Pds {

  class MyL1Server : public ToEb, public EbCountSrv {
  public:
    MyL1Server(const Src& s) : ToEb(s) {}
    unsigned count() const {return *reinterpret_cast<const unsigned*>(&sequence()); }
    virtual bool succeeds (EbEventKey& key) const { return key.precedes ((const EbCountSrv&)*this); }
    virtual bool coincides(EbEventKey& key) const { return key.coincides((const EbCountSrv&)*this); }
    virtual void assign   (EbEventKey& key) const { key.assign   ((const EbCountSrv&)*this); }
  };

  //
  //  This class creates the server when the streams are connected.
  //  Real implementations will have something like this.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(const Src& src) :
      _src(src)
    {}
    virtual ~MySegWire() 
    {
    }
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface);
    MyL1Server* server() { return _srvC; }
  private:
    Src         _src;
    MyL1Server* _srvC;
  };

  //
  //  This class derives from SegmentLevel in order to
  //  simulate a device driver by creating a datagram on 
  //  a socket in response to L1Accepts issued by the ControlLevel.
  //
  class MyDriver : public SegmentLevel {
  enum { PayloadSize = 4*1024*1024 };

  public:
    MyDriver(unsigned platform, int size1, int size2,
	     MySegWire& settings, EventCallback& cb, Arp* arp) : 
      SegmentLevel(platform, settings, cb, arp),
      _size0   (size1),
      _dsize   (size2-size1),
      _seg_wire(settings),
      _server(0),
      _pool(sizeof(CDatagram)+PayloadSize,4)
    {
      for(unsigned k=0; k<PayloadSize; k++)
	_payload[k] = k&0xff;
    }
    ~MyDriver() {}

    bool attach()
    {
      bool result = SegmentLevel::attach();
      _server = _seg_wire.server();
      return result;
    }

  private:
    void message(const Node& hdr, const Message& msg)
    {
      if (hdr.level() == Level::Control) {
	if (msg.type() == Message::Transition) {
	  const Transition& tr = reinterpret_cast<const Transition&>(msg);
	  if (tr.id() == TransitionId::L1Accept &&
	      tr.phase() == Transition::Record) {
	    if (_server) {
	      CDatagram* cdg = new(&_pool) CDatagram(TypeId(TypeNum::Any),
						     _server->client());
	      //  Because I use a ToEb as a simulated source of data, 
	      //  I need to add the Xtc header within the payload.
	      Datagram& dg = const_cast<Datagram&>(cdg->datagram());
	      Xtc* xtc = new(&dg.xtc) Xtc(TypeNum::Any, _server->client());
	      int sz = (_dsize==0) ? 
		_size0 :
		_size0 + ((random()%_dsize) & ~3);
	      char* p = &_payload[(random()%(PayloadSize-sz)) & ~3];
	      memcpy(dg.xtc.alloc(sz),p,sz);
	      xtc->alloc(sz);
	      *reinterpret_cast<unsigned*>(&dg) = _evr;
	      _server->send(cdg);
	      _evr++;
	    }
	  }
	  else
	    _evr = 0;
	}
      }
      SegmentLevel::message(hdr,msg);
    }
  private:
    int        _size0;
    int        _dsize;
    MySegWire& _seg_wire;
    MyL1Server* _server;
    unsigned   _evr;
    char       _payload[PayloadSize];
    GenericPoolW _pool;
  };

  //
  //  Sample feature extraction.
  //  Iterate through the input datagram to strip out the BLD
  //  and copy the remaining data.
  //
  class MyIterator : public XtcIterator {
  public:
    MyIterator(Datagram& dg) : _dg(dg) {}
    int process(const Xtc& xtc,
		InDatagramIterator* iter)
    {
      if (xtc.contains==TypeNum::Id_Xtc)
	return iterate(xtc,iter);
      if (xtc.src.phy() == detid) {
	_dg.xtc.damage.increase(xtc.damage.value());
	memcpy(_dg.xtc.alloc(sizeof(Xtc)),&xtc,sizeof(Xtc));
	int size = xtc.sizeofPayload();
	iovec iov[1];
	int remaining = size;
	while( remaining ) {
	  int sz = iter->read(iov,1,size);
	  if (!sz) break;
	  memcpy(_dg.xtc.alloc(sz),iov[0].iov_base,sz);
	  remaining -= sz;
	} 
	return size-remaining;
      }
      return 0;
    }
  private:
    Datagram& _dg;
  };

  class MyFEX : public Appliance {
  public:
    enum { PoolSize = 32*1024*1024 };
    enum { EventSize = 2*1024*1024 };
  public:
    MyFEX() : _pool(PoolSize,EventSize), 
	      _iter(sizeof(ZcpDatagramIterator),32) {}
    ~MyFEX() {}

    Transition* transitions(Transition* tr) 
    {
      return tr;
    }
    InDatagram* occurrences(InDatagram* input) { return input; }
    InDatagram* events     (InDatagram* input) 
    {
      if (verbose) {
	InDatagramIterator* in_iter = input->iterator(&_iter);
	int advance;
	Browser browser(input->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }
      return input;

      if (input->datagram().seq.type()   !=Sequence::Event ||
	  input->datagram().seq.service()!=TransitionId::L1Accept) {
	if (verbose) {
	  printf("events %p  type %d service %d\n",
		 input,
		 input->datagram().seq.type(),
		 input->datagram().seq.service());
	  InDatagramIterator* in_iter = input->iterator(&_iter);
	  int advance;
	  Browser browser(input->datagram(), in_iter, 0, advance);
	  browser.iterate();
	  delete in_iter;
	}
	return input;
      }

      CDatagram* ndg = new (&_pool)CDatagram(input->datagram());

      if (verbose) {
	printf("new cdatagram %p\n",ndg);
	InDatagramIterator* in_iter = input->iterator(&_iter);
	int advance;
	Browser browser(input->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }
      {
      InDatagramIterator* in_iter = input->iterator(&_iter);
      MyIterator iter(const_cast<Datagram&>(ndg->datagram()));
      iter.iterate(input->datagram().xtc,in_iter);
      delete in_iter;
      }
      if (verbose) {
	InDatagramIterator* in_iter = ndg->iterator(&_iter);
	int advance;
	Browser browser(ndg->datagram(), in_iter, 0, advance);
	browser.iterate();
	delete in_iter;
      }

      _pool.shrink(ndg, ndg->datagram().xtc.sizeofPayload()+sizeof(Datagram));
      return ndg;
    }
  private:
    RingPool _pool;
    GenericPool _iter;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class SegTest : public EventCallback {
  public:
    SegTest(Task*                 task,
	    unsigned              platform,
	    SegWireSettings&      settings,
	    Arp*                  arp) :
      _task    (task),
      _platform(platform)
    {
    }

    virtual ~SegTest()
    {
      _task->destroy();
    }
    

  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("SegTest connected to platform 0x%x\n", 
	     _platform);

      Stream* frmk = streams.stream(StreamParams::FrameWork);
      (new MyFEX)->connect(frmk->inlet());
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
    Task*         _task;
    unsigned      _platform;
  };
}

using namespace Pds;

void MySegWire::connect (InletWire& wire,
			 StreamParams::StreamType s,
			 int interface)
{
  //  Build the L1 Data server
  _srvC = new MyL1Server(_src);
  wire.add_input(_srvC);
  printf("Assign l1d %d (%p)\n",_srvC->id(),_srvC);

  //  (new MyFEX)->connect(&dynamic_cast<InletWireServer*>(&wire)->_inlet);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = 0;
  int      size1    = 1024;
  int      size2    = 1024;
  Arp* arp = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:s:v")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
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
      sprintf(message, "failed to create odfArp : %s", 
	      strerror(arp->error()));
      printf("%s %s\n",argv[0], message);
      delete arp;
      return 0;
    }
  }

  Task* task = new Task(Task::MakeThisATask);
  MySegWire settings(Src(Node(Level::Source,platform), detid));
  SegTest* segtest = new SegTest(task, platform, settings, arp);
  MyDriver* driver = new MyDriver(platform, size1, size2,
				  settings, *segtest, arp);
  if (driver->attach())
    task->mainLoop();

  driver->detach();
  if (arp) delete arp;
  return 0;
}
