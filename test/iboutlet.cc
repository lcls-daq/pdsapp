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

#include "pdsapp/test/ibcommon.hh"
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//#define VERBOSE

static const unsigned short outlet_port = 11000;
static const unsigned max_dg_size = 0x4000;

static void dump(const Pds::CDatagram* dg)
{
  char buff[128];
  time_t t = dg->seq.clock().seconds();
  strftime(buff,128,"%H:%M:%S",localtime(&t));
  printf("[%p] %s.%09u %016lx %s extent 0x%x damage %x\n",
         dg,
         buff,
         dg->seq.clock().nanoseconds(),
         dg->seq.stamp().fiducials(),
         Pds::TransitionId::name(Pds::TransitionId::Value((dg->seq.stamp().fiducials()>>24)&0xff)),
         dg->xtc.extent, dg->xtc.damage.value());
}

namespace Pds {

  class XtcFileIteratorC {
  public:
    XtcFileIteratorC(int fd, size_t maxDgramSize) :
      _fd(fd), _maxDgramSize(maxDgramSize), _buf(maxDgramSize,8) 
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
      while( (p=_buf.alloc(1))==0 ) ;
      _buf.free(p);
      CDatagram* _cdg = new (&_buf)CDatagram(Dgram());
      Datagram& dg = _cdg->datagram();
      if (::read(_fd, &dg, sizeof(dg))==0) return 0;
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
      
      return sz!=(ssize_t)payloadSize ? 0: _cdg;
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
  };

  class IbOutletWire : public OutletWire,
                       public RdmaSlaveCb {
  public:
    IbOutletWire(Outlet& o,
                 const XtcFileIteratorC& iter) :
      OutletWire(o),
      _rdma(new IbRdmaSlave(iter.poolBegin(),
                            iter.poolSize (),
                            sizeof(Datagram),
                            Ins(outlet_port),
                            *this))
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
      //  Send the dg header, xtc addr
      _rdma->req_read(reinterpret_cast<const char*>(&dg->datagram()),
                      dg->datagram().xtc.payload(),
                      dg->datagram().xtc.sizeofPayload());
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
      delete reinterpret_cast<CDatagram*>(b-sizeof(Datagram));
    }
  private:
    IbRdmaSlave* _rdma;
    Ins     _bcast;
    int     _control;
  };


};

using namespace Pds;

int main(int argc, char* argv[])
{
  const char* filename = 0;
  unsigned   partition = 0;

  int c;
  //  bool lUsage = false;
  while ( (c=getopt( argc, argv, "f:p:h")) != EOF ) {
    switch(c) {
    case 'f':
      filename = optarg;
      break;
    case 'p':
      partition = strtoul(optarg,NULL,0);
      break;
    default:
      break;
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

  while ((cdg = iter.next())) {
    dump(cdg);
    inlet->post(cdg);
  }

  return 1;
}
