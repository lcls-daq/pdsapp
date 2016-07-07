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

#include "ibcommon.hh"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/Datagram.hh"

#include <unistd.h>
#include <stdio.h>
#include <time.h>

//#define VERBOSE

static const unsigned short outlet_port = 11000;

static void dump(const Pds::Datagram* dg)
{
  char buff[128];
  time_t t = dg->seq.clock().seconds();
  strftime(buff,128,"%H:%M:%S",localtime(&t));
  printf("%s.%09u %016lx %s extent 0x%x damage %x\n",
         buff,
         dg->seq.clock().nanoseconds(),
         dg->seq.stamp().fiducials(),
         Pds::TransitionId::name(Pds::TransitionId::Value((dg->seq.stamp().fiducials()>>24)&0xff)),
         dg->xtc.extent, dg->xtc.damage.value());
}

namespace Pds {

  class MasterCallback : public RdmaMasterCb {
  public:
    MasterCallback(const Ins& ins) :
      _pool(0x4000,12)
      {
        char* p0 = (char*)_pool.alloc(1);
        _pool.free(p0);
        _rdma = new IbRdmaMaster(p0-sizeof(PoolEntry),
                                 _pool.numberofObjects()*(_pool.sizeofObject()+sizeof(PoolEntry)),
                                 sizeof(Datagram),
                                 ins,
                                 *this);
      }
  public:
    void* request(const char* hdr,
                  unsigned    payload_size)
    {
      void* payload = _pool.alloc(payload_size);
#ifdef VERBOSE
      printf("Request %u to %p\n",payload_size, payload);

      const Datagram* dg = reinterpret_cast<const Datagram*>(hdr);
      dump(dg);
      printf("Contains %s : target %p\n",
             TypeId::name(dg->xtc.contains.id()),
             payload);
#else
      printf("[%d] ",_pool.numberofAllocs()-_pool.numberofFrees());
      const Datagram* dg = reinterpret_cast<const Datagram*>(hdr);
      dump(dg);
#endif
      return payload;
    }
    void  complete(void* payload)
    {
#ifdef VERBOSE
      printf("Payload %p complete\n",payload);
      uint32_t* p = reinterpret_cast<uint32_t*>(payload);
      for(unsigned i=0; i<32; i++)
        printf("%08x%c",p[i],(i%8)==7?'\n':' ');
      printf("\n");
#endif
      _pool.free(payload);
    }
  private:
    GenericPool   _pool;
    IbRdmaMaster* _rdma;
  };
};

using namespace Pds;

int main(int argc, char* argv[])
{
  const char* addr = "192.168.0.1";
  int c;
  while ( (c=getopt( argc, argv, "a:h")) != EOF ) {
    switch(c) {
    case 'a':
      addr = optarg;
      break;
    default:
      break;
    }
  }

  in_addr ia;
  inet_aton(addr,&ia);

  MasterCallback cb(Ins(ntohl(ia.s_addr),outlet_port));
  
  while(1) {
    sleep(1);
  }
  return 1;
}
