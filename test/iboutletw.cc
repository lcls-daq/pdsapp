/********************************************************************
 *  Program to test an Infiniband outlet wire from a segment level  *
 *
 *  1)  Open IB device
 *  2)  Register datagram pool memory regions with IB device
 *  3)  Open TCP connection to each event level
 *      - exchange IB context
 *  4)  Write Dg header and addr to TCP connection
 *  5)  Listen for addr to return datagram pool memory
 ********************************************************************/

#include "pdsapp/test/ibcommonw.hh"
#include "pds/utility/Inlet.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/OutletWire.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPoolW.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"

#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

using namespace Pds::IbW;

//#define VERBOSE

static const unsigned short outlet_port = 11000;
static const unsigned max_dg_size = 0x400000;
static unsigned _tare = 0;

static void dump(const Pds::CDatagram* dg)
{
  unsigned trid = dg->seq.service();
  if (trid != 0xc) {
    char buff[128];
    time_t t = dg->seq.clock().seconds();
    strftime(buff,128,"%H:%M:%S",localtime(&t));
    printf("[%p] %s.%09u %016lx %s extent 0x%x damage %x\n",
           dg,
           buff,
           dg->seq.clock().nanoseconds(),
           dg->seq.stamp().fiducials(),
           Pds::TransitionId::name(Pds::TransitionId::Value(trid)),
           dg->xtc.extent, dg->xtc.damage.value());
  }
}

static double tdiff(const struct timespec& tv_b,
                    const struct timespec& tv_e)
{ return double(tv_e.tv_sec-tv_b.tv_sec)+1.e-9*(double(tv_e.tv_nsec)-double(tv_b.tv_nsec)); }
  
namespace Pds {

  class HistoTime {
  public:
    HistoTime(const char* name,
              unsigned    bins,
              double      range) :
      _name   (name),
      _bins   (bins),
      _range  (range),
      _entries(new uint32_t[bins+1]) 
    { memset(_entries,0,(bins+1)*sizeof(uint32_t));}
    ~HistoTime()
    { delete[] _entries; }
  public:
    void fill(double t) { 
      if (t<_range) _entries[unsigned(double(_bins)*t/_range)]++;
      else          _entries[_bins]++;
    }
    void dump() const {
      printf("-- %s -- [%f sec] [%u overflows]\n",
             _name.c_str(),_range,_entries[_bins]);
      for(unsigned i=0; i<_bins; i++)
        printf("%8u%c",_entries[i],(i%10)==9?'\n':' ');
      printf("\n");
    }
  private:
    std::string _name;
    unsigned    _bins;
    double      _range;
    unsigned*   _entries;
  };

  class XtcFileIteratorC {
  public:
    XtcFileIteratorC(int fd, size_t maxDgramSize) :
      _fd(fd), _maxDgramSize(maxDgramSize), _buf(maxDgramSize,8),
      _bufh ("No Buffer",100,1.e-3),
      _readh("Read",100,2.e-3)
    {
      char* p0 = (char*)_buf.alloc(1);
      char* p1 = (char*)_buf.alloc(1);
      _poolBegin = p0-sizeof(PoolEntry);
      _poolSize  = (_buf.sizeofObject()+sizeof(PoolEntry))*_buf.numberofObjects();
      _buf.free(p0);
      _buf.free(p1);
    }
    ~XtcFileIteratorC() {}
  public:
    CDatagram* next() {
      void* p;
      struct timespec tv_b; 
      struct timespec tv_e; 
      clock_gettime(CLOCK_REALTIME,&tv_b);
      while( (p=_buf.alloc(1))==0 ) ;
      clock_gettime(CLOCK_REALTIME,&tv_e);
      _bufh.fill(tdiff(tv_b,tv_e));

      _buf.free(p);
      CDatagram* _cdg = new (&_buf)CDatagram(Dgram());
      Datagram& dg = _cdg->datagram();
      if (::read(_fd, &dg, sizeof(dg))==0) {
        _bufh.dump();
        _readh.dump();
        return 0;
      }

      size_t payloadSize = dg.xtc.sizeofPayload();
      if ((payloadSize+sizeof(*_cdg))>_maxDgramSize) {
	printf("Datagram size %zu larger than maximum: %zu\n", payloadSize+sizeof(*_cdg), _maxDgramSize);
	return 0;
      }
      ssize_t sz = ::read(_fd, dg.xtc.payload(), payloadSize);
      if (sz != (ssize_t)payloadSize) {
	printf("XtcFileIterator::next read incomplete payload %zd/%zd\n",
	       sz,payloadSize);
      } 
      clock_gettime(CLOCK_REALTIME,&tv_b);
      _readh.fill(tdiff(tv_e,tv_b));

      if (sz!=(ssize_t)payloadSize) {
        return 0;
      }
      else
        return _cdg;
    }
  public:
    char*    poolBegin() const { return _poolBegin; }
    unsigned poolSize () const { return _poolSize ; }
  private:
    int          _fd;
    size_t       _maxDgramSize;
    GenericPool  _buf;
    char*        _poolBegin;
    unsigned     _poolSize;
    HistoTime    _bufh;
    HistoTime    _readh;
  };

  class IbOutletWire : public OutletWire,
                       public RdmaMasterCb {
  public:
    IbOutletWire(Outlet& o,
                 const XtcFileIteratorC& iter) :
      OutletWire(o),
      _rdma(new RdmaMaster(iter.poolBegin(),
                           iter.poolSize (),
                           Ins(outlet_port),
                           *this)),
      _index(0)
    {
    }
  public:
    Transition* forward(Transition* tr) {
      //  Unicast
      return tr;
    }
    Occurrence* forward(Occurrence* occ) {
      //  Unicast
      return occ;
    }
    InDatagram* forward(InDatagram* dg) {
      reinterpret_cast<uint32_t*>(dg->datagram().xtc.next())[-1] = _index;
      //  Send the dg header, xtc addr
      _rdma->req_write((void*)(&dg->datagram()),
                       dg->datagram().xtc.sizeofPayload()+sizeof(Datagram)+_tare,
                       _index++);
      if (_index==12) _index=0;
      return (InDatagram*)Appliance::DontDelete;
    }

    void bind(NamedConnection, const Ins& ins) {
      _bcast = ins;
    }
    void bind(unsigned id    , const Ins& ins) {
    }
    void unbind(unsigned id) {
    }
  public:
    void complete(void* p) {  
#ifdef VERBOSE
      printf("complete %p\n",p);
#endif
      char* b = (char*)p;
      delete reinterpret_cast<CDatagram*>(b);
    }
  private:
    RdmaMaster* _rdma;
    Ins     _bcast;
    int     _control;
    unsigned    _index;
  };


};

using namespace Pds;

static void show_usage(const char* p)
{
  printf("Usage: %s -f <xtc file path> [options]\n",p);
  printf("Options: -p <partition>\n");
}

int main(int argc, char* argv[])
{
  const char* filename = 0;
  unsigned   partition = 0;

  int c;
  //  bool lUsage = false;
  while ( (c=getopt( argc, argv, "f:p:t:xh")) != EOF ) {
    switch(c) {
    case 'f':
      filename = optarg;
      break;
    case 'p':
      partition = strtoul(optarg,NULL,0);
      break;
    case 'h':
      show_usage(argv[0]);
      return 0;
    case 't':
      _tare = strtoul(optarg,NULL,0);
      break;
    default:
      printf("Unknown option: -%c\n",c);
      return -1;
    }
  }

  if (!filename) {
    printf("Argument -f <filename> required\n");
    return -1;
  }

  int fd = open(filename, O_RDONLY | O_LARGEFILE);
  if (fd < 0) {
    perror("Unable to open file\n");
    return -2;
  }

  XtcFileIteratorC iter(fd,max_dg_size);

  Inlet*      inlet = new Inlet;
  Outlet*    outlet = new Outlet;
  outlet->connect(inlet);
  IbOutletWire* o = new IbOutletWire(*outlet, iter);

  o->bind(OutletWire::Bcast, StreamPorts::bcast(partition,
                                                Level::Control,
                                                0));

  CDatagram* cdg;

  timespec tb; clock_gettime(CLOCK_REALTIME,&tb);
  uint64_t bb=0;
  uint32_t ne=0;

  while ((cdg = iter.next())) {
    dump(cdg);
    ne++;
    bb += cdg->xtc.sizeofPayload()+sizeof(*cdg);
    inlet->post(cdg);
  }

  timespec te; clock_gettime(CLOCK_REALTIME,&te);

  double td = tdiff(tb,te);
  printf("--Total %lu bytes  (%u dgs)  %f sec  %f Gb/sec\n",
         bb, ne, td, 8.e-9*double(bb)/td);

  return 1;
}
