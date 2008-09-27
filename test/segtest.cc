#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"   // SegmentOptions
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/service/ZcpFragment.hh"
#include "pds/utility/EbServer.hh"
#include "pds/utility/EbTimeouts.hh"
#include "pds/utility/EvrServer.hh"
#include "pds/utility/ToEb.hh"
#include "pds/utility/ToEbWire.hh"
#include "pds/utility/EbC.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/xtc.hh"
#include "pds/collection/Transition.hh"
#include "pds/client/InXtcIterator.hh"
#include "pds/client/Browser.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

namespace Pds {

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
		  int interface)
    {
      //  Build the L1 Data server
      _srvC = new ToEb(_src);
      wire.add_input(_srvC);
      printf("Assign l1d %d\n",_srvC->id());
    }
    ToEb* server() { return _srvC; }
  private:
    Src        _src;
    ToEb*      _srvC;
  };

  //
  //  This class derives from SegmentLevel in order to
  //  simulate a device driver by creating a datagram on 
  //  a socket in response to L1Accepts issued by the ControlLevel.
  //
  class MyDriver : public SegmentLevel {
  enum { PayloadSize = 1024 };

  public:
    MyDriver(unsigned partition, 
	     int      index,
	     MySegWire& settings, EventCallback& cb, Arp* arp) : 
      SegmentLevel(partition, index, settings, cb, arp),
      _seg_wire(settings),
      _server(0),
      _evr(*(unsigned*)&_datagram.datagram()),
      _datagram(TC(TypeNumPrimary::Id_XTC),
		Src(Level::Segment,-1UL,
		    0xa5a5a5a5))
    {
      for(unsigned k=0; k<PayloadSize; k++)
	_payload[k] = k&0xff;
      const_cast<Datagram&>(_datagram.datagram()).xtc.tag.extend(PayloadSize);
    }
    ~MyDriver() {}

  private:
    void message(const Node& hdr, const Message& msg)
    {
      if (hdr.level() == Level::Control) {
	if (msg.type() == Message::Transition) {
	  const Transition& tr = reinterpret_cast<const Transition&>(msg);
	  if (tr.id() == Transition::L1Accept &&
	      tr.phase() == Transition::Record) {
	    if (_server) {
	      _server->send(&_datagram);
	      _evr++;
	    }
	  }
	  else
	    _evr = 0;
	}
      }
      SegmentLevel::message(hdr,msg);
    }
    void connected   (const Node& hdr, const Message& msg)
    {
      SegmentLevel::connected(hdr,msg);
      _server = _seg_wire.server();
    }

  private:
    MySegWire& _seg_wire;
    ToEb*  _server;
    unsigned&  _evr;
    CDatagram  _datagram;
    char       _payload[PayloadSize];
  };

  //
  //  Sample feature extraction.
  //  Iterate through the input datagram to strip out the BLD
  //  and copy the remaining data.
  //
  class MyIterator : public InXtcIterator {
  public:
    MyIterator(Datagram& dg) : _dg(dg) {}
    int process(const InXtc& xtc,
		InDatagramIterator* iter)
    {
      if (xtc.tag.contains()==TypeNumPrimary::Id_InXtcContainer)
	return iterate(xtc,iter);
      if (xtc.src.detector() == 0xa5a5) {
	memcpy(_dg.xtc.tag.alloc(sizeof(InXtc)),&xtc,sizeof(InXtc));
	int size = xtc.sizeofPayload();
	iovec iov[1];
	int remaining = size;
	while( remaining ) {
	  int sz = iter->read(iov,1,size);
	  if (!sz) break;
	  memcpy(_dg.xtc.tag.alloc(sz),iov[0].iov_base,sz);
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
      if (input->datagram().type()!=Sequence::Event ||
	  input->datagram().service()!=Sequence::L1Accept)
	return input;

      CDatagram* ndg = new (&_pool)CDatagram(input->datagram());
      /*
      printf("new cdatagram %p\n",ndg);
      {
      InDatagramIterator* in_iter = input->iterator(&_iter);
      int advance;
      Browser browser(input->datagram(), in_iter, 0, advance);
      browser.iterate();
      delete in_iter;
      }
      */
      {
      InDatagramIterator* in_iter = input->iterator(&_iter);
      MyIterator iter(const_cast<Datagram&>(ndg->datagram()));
      iter.iterate(input->datagram().xtc,in_iter);
      delete in_iter;
      }
      /*
      {
      InDatagramIterator* in_iter = ndg->iterator(&_iter);
      int advance;
      Browser browser(ndg->datagram(), in_iter, 0, advance);
      browser.iterate();
      delete in_iter;
      }
      */
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
	    unsigned              partition,
	    SegWireSettings&      settings,
	    Arp*                  arp) :
      _task(task),
      _segment(0)
    {
    }

    virtual ~SegTest()
    {
      if (_segment) delete _segment;
      _task->destroy();
    }
    
    void attach(SegmentLevel* segment)
    {
      _segment = segment;
      _segment->attach();
    }

  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("SegTest connected to partition 0x%x\n", 
	     _segment->header().partition());

      Stream* frmk = streams.stream(StreamParams::FrameWork);
      (new MyFEX)->connect(frmk->inlet());
      //      (new Decoder)->connect(frmk->inlet());
    }
    void failed(Reason reason)
    {
      static const char* reasonname[] = { "platform unavailable", 
					  "crates unavailable", 
					  "fcpm unavailable" };
      printf("SegTest: unable to allocate crates on partition 0x%x : %s\n", 
	     _segment->header().partition(), reasonname[reason]);
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
      
      printf("SegTest: partition 0x%x dissolved by user %s, pid %d, on node %s", 
	     who.partition(), username, who.pid(), ipname);
      
      delete this;
    }
    
  private:
    Task*         _task;
    SegmentLevel* _segment;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned partition = 0;
  unsigned index     = 0;
  unsigned source = 0;
  Arp* arp = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:s:")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      index  = strtoul(optarg, NULL, 0);
      break;
    case 's':
      source = strtoul(optarg, NULL, 0);
      break;
    case 'p':
      partition = strtoul(optarg, NULL, 0);
      break;
    }
  }

  if (!partition) {
    printf("%s: partition mask required\n",argv[0]);
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
  MySegWire settings(Src(Level::Segment,-1UL,source));
  SegTest* segtest = new SegTest(task, partition, settings, arp);
  MyDriver* driver = new MyDriver(partition, index,
				  settings, *segtest, arp);
  segtest->attach(driver);

  task->mainLoop();
  if (arp) delete arp;
  return 0;
}
