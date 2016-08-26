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

#include "ibcommonw.hh"
#include "pds/xtc/Datagram.hh"

#include <unistd.h>
#include <stdio.h>
#include <time.h>

using namespace Pds::IbW;

static bool lverbose = false;

static const unsigned max_payload_size = 0x400000;

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

  class SlaveCallback : public IbW::RdmaSlaveCb {
  public:
    SlaveCallback(const Ins& ins) :
      _poolv(12),
      _index( 0),
      _decade(10)
      {
        char* p = new char[max_payload_size*_poolv.size()];
        for(unsigned i=0; i<_poolv.size(); i++)
          _poolv[i] = p+max_payload_size*i;

        unsigned sz = max_payload_size*_poolv.size();
        _rdma = new IbW::RdmaSlave(p, sz,
                                   _poolv,
                                   ins, *this);
      }
  public:
    void  complete(void* payload)
    {
      if (lverbose) {
        printf("Payload %p complete\n",payload);
        uint32_t* p = reinterpret_cast<uint32_t*>(payload);
        for(unsigned i=0; i<32; i++)
          printf("%08x%c",p[i],(i%8)==7?'\n':' ');
        printf("\n");
      }
      Dgram* dg = (Dgram*)payload;
      uint32_t index = reinterpret_cast<uint32_t*>((char*)dg->xtc.next())[-1];
      if (index!=(_index%12))
        printf("Expected %x, found %x\n",_index,index);
      if ((++_index%_decade)==0) {
        printf("Evt %u  [extent 0x%x] idx %u\n",_index,dg->xtc.extent,index);
        if ((_index%(10*_decade))==0)
          _decade*=10;
      }
    }
  private:
    std::vector<char*> _poolv;
    unsigned           _index;
    unsigned           _decade;
    IbW::RdmaSlave*    _rdma;
  };
};

using namespace Pds;

static void show_usage(const char* p)
{
  printf("Usage: %s -a <Server IP address: dotted notation> [-v]\n",p);
}

int main(int argc, char* argv[])
{
  const char* addr = "192.168.0.1";
  int c;
  while ( (c=getopt( argc, argv, "a:hv")) != EOF ) {
    switch(c) {
    case 'a':
      addr = optarg;
      break;
    case 'h':
      show_usage(argv[0]);
      return 0;
    case 'v':
      lverbose = true;
      break;
    default:
      printf("Unknown option: -%c\n",c);
      return -1;
    }
  }

  in_addr ia;
  inet_aton(addr,&ia);

  SlaveCallback cb(Ins(ntohl(ia.s_addr),outlet_port));
  
  while(1) {
    sleep(1);
  }
  return 1;
}
