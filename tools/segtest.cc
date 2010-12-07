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
#include "pds/collection/Route.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

//#define USE_ZCP

bool verbose = false;

namespace Pds {

  enum {v_fiduc =  0, k_fiduc = TimeStamp::NumFiducialBits};
  enum {m_fiduc = ((1 << k_fiduc)-1), s_fiduc = (m_fiduc <<v_fiduc)};

  class MySeqServer;
  MySeqServer* mySeqServerGlobal = NULL;
  int       pipefd[2];
  unsigned rateInCPS = 1;
  unsigned seconds = 1;

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

  long long int timeDiff(timespec* end, timespec* start) {
    long long int diff;
    diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
    diff += end->tv_nsec;
    diff -= start->tv_nsec;
    return diff;
  }

  class MySeqServer : public EvrServer, private Routine {
    public:
    MySeqServer(unsigned   platform, 
                int        fd,
                InletWire& inlet) :
          EvrServer(StreamPorts::event(platform,Level::Segment),
                    DetInfo(-1UL,DetInfo::NoDetector,0,DetInfo::Evr,0),
                    inlet, 4),
              _task(new Task(TaskObject("segtest_evr",127))),
              _outlet(sizeof(EvrDatagram),0, Ins(Route::interface())),
              _go(false),
              _pipe(fd),
              _evr(0),
              period(1000000000LL),
              _f(0), _f_increment(360*seconds/rateInCPS), _t(0)
              { _task->call(this); }

    ~MySeqServer() { _task->destroy(); }

    void routine() 
    {
      _sleepTime.tv_sec = 0;
      _sleepTime.tv_nsec = 0;
      _waitTime.tv_sec = 0;
      _waitTime.tv_nsec = 250000000;
      unsigned trips = 0;
      if (seconds > 1) {
        rateInCPS = 1;
        period = 1000000000LL * seconds;
      } else if (rateInCPS > 1) {
        seconds = 0;
        period = 1000000000LL / rateInCPS;
      }
      printf("Period is %llu nsec\n", period);
      ServerMsg dg;
      while(1) {
        while (!_go) {
          if (nanosleep(&_waitTime, &_fooTime)<0) perror("nanosleep while spinning");
          trips = 0;
        }
        // generate these at specified interval
        clock_gettime(CLOCK_REALTIME, &_start);
        dg.offset = 0;
        dg.ptr    = 0;
        dg.length = 0;
        dg.evr.seq = Sequence(Sequence::Event, TransitionId::L1Accept, ClockTime(_start.tv_sec, _start.tv_nsec), TimeStamp(_t, _f, _evr));
        dg.evr.evr = _evr++;
        ::write(_pipe,&dg,sizeof(dg));
        _outlet.send((char*)&dg,0,0,_dst);
        _f += _f_increment;
        _f &= Pds::s_fiduc;  // mask it to correct number of bits
        _t = (random() & 0x3) + 12;
        clock_gettime(CLOCK_REALTIME, &_done);
        _busyTime = timeDiff(&_done, &_start);
        if (seconds) _sleepTime.tv_sec = seconds;
        else {
          if (period > _busyTime)  _sleepTime.tv_nsec = period - _busyTime;
        }
        if (nanosleep(&_sleepTime, &_fooTime)<0) perror("nanosleep");
        if (trips++ < 10) printf ("busy time %llu nsec,   sleep time %lu sec and %lu nsec\n", _busyTime, _sleepTime.tv_sec,  _sleepTime.tv_nsec);
      }
    }
    void dst(const Ins& ins) { _dst=ins; }
    void enable() { _go = true; printf("going\n");}
    void disable() { _go = false; printf("stopping, evr %d\n", _evr);}

    private:
    Task* _task;
    Client _outlet;
    bool _go;
    int   _pipe;
    unsigned _evr;
    timespec _start, _done, _sleepTime, _fooTime, _waitTime;
    long long unsigned period, _busyTime;
    unsigned _f, _f_increment;
    unsigned _t;
    Ins _dst;
  };

  class MyL1Server : public EbServer, EbCountSrv {
      enum { PayloadSize = 1024 };
    public:
      MyL1Server(unsigned platform,
          int size1, int size2,
          const Src& s) :
            _xtc(TypeId(TypeId::Any,0), s),
            _size0   (size1),
            _dsize   (size2-size1),
            _count(0)
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
        ::pipe(pipefd);
        fd(pipefd[0]);

            }
      ~MyL1Server() { }
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
        ::read(pipefd[0],&_hdr,sizeof(_hdr));
        _xtc.extent = PayloadSize + sizeof(Xtc);
        memcpy(payload, &_xtc, sizeof(_xtc));
        memcpy(payload+sizeof(_xtc),_payload,PayloadSize);
        _hdr.length = _xtc.extent;
        return _hdr.length;
      }
      int      fetch       (ZcpFragment& zf, int flags)
      {
        ::read(pipefd[0],&_hdr,sizeof(_hdr));
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
          ::write(pipefd[1],&hdr,sizeof(hdr));
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
            ::write(pipefd[1],&hdr,sizeof(hdr));
          }
          return len;
        }
      }
    public:
      unsigned        count() const { return _hdr.evr.evr; }
    private:
      int     _fetch       (char** payload)
      {
        int sz = PayloadSize;
        *payload = _payload;
        return sz;
      }
    private:
      ServerMsg    _hdr;
      Xtc       _xtc;
      bool      _more;
      int       _size0;
      int       _dsize;
      char      _payload[PayloadSize];
      char      _evrPayload[sizeof(EvrDatagram)];
      ZcpFragment _zfragment;
      ZcpStream   _zpayload;
      unsigned _count;
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
        //verbose   = in->env().value() >= NumberOf;
        printf("MyFex::algorithm %d  verbose %c\n",
            _algorithm,verbose ? 't':'f');
        return in;
      }
      InDatagram* configure(InDatagram* in) {return in;}
      Transition* l1accept (Transition* in) {return in;}

      InDatagram* l1accept (InDatagram* input)
      {
        if (verbose) {
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
      RingPool    _pool;
      GenericPool _iter;
      Datagram*   _outdg;
  };

  class L1Action : public Action {
    public:
      L1Action(MyFEX& fex) : _fex(fex) {}
      Transition* fire(Transition* tr) { return tr; }
      InDatagram* fire(InDatagram* dg) { return dg; }
    private:
      MyFEX& _fex;
  };

  class ConfigAction : public Action {
    public:
      ConfigAction(MyFEX& fex) : _fex(fex) {}
      Transition* fire(Transition* tr) { return _fex.configure(tr); }
      //InDatagram* fire(InDatagram* dg) { return _fex.configure(dg); }
    private:
      MyFEX& _fex;
  };

  class BeginRunAction : public Action {
    public:
      Transition* fire(Transition* tr) { return tr; }
      //InDatagram* fire(InDatagram* tr) { return tr; }
  };

  class EndRunAction : public Action {
    public:
      Transition* fire(Transition* tr) { return tr; }
      //InDatagram* fire(InDatagram* tr) { return tr; }
  };

  class AllocAction : public Action {
    public:
      Transition* fire(Transition* tr) {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        mySeqServerGlobal->dst(StreamPorts::event(alloc.allocation().partitionid(),
            Level::Segment));
        return tr;
      }
  };

  class EnableAction : public Action {
    public:
      Transition* fire(Transition* tr) {
        mySeqServerGlobal->enable();
        return tr;
      }
  };

  class DisableAction : public Action {
    public:
      Transition* fire(Transition* tr) {
        mySeqServerGlobal->disable();
        return tr;
      }
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
        _inlet = &wire;
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
        fsm->callback(TransitionId::Map        , new AllocAction);
        fsm->callback(TransitionId::Configure  , new ConfigAction(*_fex));
        fsm->callback(TransitionId::Enable     , new EnableAction);
        fsm->callback(TransitionId::BeginRun   , new BeginRunAction);
        fsm->callback(TransitionId::L1Accept   , new L1Action    (*_fex));
        fsm->callback(TransitionId::EndRun     , new EndRunAction);
        fsm->callback(TransitionId::Disable    , new DisableAction);
        fsm->connect(frmk->inlet());
        mySeqServerGlobal  = new MySeqServer(_platform, pipefd[1], *_inlet);
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
      InletWire*  _inlet;
  };
}

using namespace Pds;

void _print_help(const char* p0)
{
  printf("Usage : %s -p <platform> [-i <det_id> -v] [(-r <rateInCPS>) | (-s <seconds>)] \n",
      p0);
}

void resTest() {
  timespec start, done, sleepTime;
  sleepTime.tv_sec = 0;
  timespec fooTime;
  long long int thisTime;
  long long int totalTime;
  bool keepGoing = true;
  for (long int s=1; s<3000000 && keepGoing; s<<=1) {
    sleepTime.tv_nsec = s;
    totalTime = 0LL;
    for (int i=0; i<100; i++) {
      clock_gettime(CLOCK_REALTIME, &start);
      if (nanosleep(&sleepTime, &fooTime)<0) {
        perror("nanosleep");
        keepGoing = false;
      }
      clock_gettime(CLOCK_REALTIME, &done);
      totalTime += timeDiff(&done, &start);
    }
    thisTime = totalTime / 100LL;
    printf("nanoSec %7ld, %7lld, %7lld\n", s, thisTime, thisTime - s);
  }
}


int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  int      size1    = 1024;
  int      size2    = 1024;
  unsigned detid = 0;
  unsigned platform = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:p:s:r:vhT")) != EOF ) {
    switch(c) {
      case 'T':
        resTest();
        break;
      case 'i':
        detid  = strtoul(optarg, NULL, 0);
        break;
      case 's':
        sscanf(optarg,"%d",&seconds);
        break;
      case 'p':
        platform = strtoul(optarg, NULL, 0);
        break;
      case 'r':
        rateInCPS = strtoul(optarg, NULL, 0);
        break;
      case 'v':
        verbose = true;
        break;
      case '?':
        printf("Unrecognized option %c\n",c);
      case 'h':
        _print_help(argv[0]);
        exit(1);
      default :
        printf("I didn't understand -%c\n", c);
        _print_help(argv[0]);
        exit(1);
    }
  }

  if (!platform) {
    printf("%s: platform required\n",argv[0]);
    _print_help(argv[0]);
    return 0;
  }

  printf("rateInCPS (%d", rateInCPS);
  if (seconds > 1) printf("/%d", seconds);
  printf(")\n");

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,platform);
  SegTest* segtest = new SegTest(task, platform, 
      size1, size2,
      DetInfo(node.pid(),DetInfo::NoDetector,
          detid,DetInfo::NoDevice,0));
  SegmentLevel* segment = new SegmentLevel(platform, 
      *segtest,
      *segtest,
      (Arp*)0);

  if (segment->attach())
    task->mainLoop();

  segment->detach();

  return 0;
}
