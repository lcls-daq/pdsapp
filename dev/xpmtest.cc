
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <new>

#include <cpsw_api_builder.h>
#include <cpsw_mmio_dev.h>
#include <cpsw_proto_mod_depack.h>

#include "pds/xpm/Module.hh"

#include <string>

//#define USE_STREAM

static inline double dtime(timespec& tsn, timespec& tso)
{
  return double(tsn.tv_sec-tso.tv_sec)+1.e-9*(double(tsn.tv_nsec)-double(tso.tv_nsec));
}

extern int optind;

using namespace Pds::Xpm;

void usage(const char* p) {
  printf("Usage: %s [-a <IP addr (dotted notation)>]\n",p);
}

int main(int argc, char** argv) {

  extern char* optarg;

  int c;
  bool lUsage = false;

  const char* ip = "192.168.2.10";
  unsigned short port = 8192;
  int fixedRate=-1;
  bool lreset=false;
  bool lenable=true;

  while ( (c=getopt( argc, argv, "a:F:RDh")) != EOF ) {
    switch(c) {
    case 'a':
      ip = optarg; break;
      break;
    case 'F':
      fixedRate = atoi(optarg);
      break;
    case 'R':
      lreset = true;
      break;
    case 'D':
      lenable = false;
      break;
    case '?':
    default:
      lUsage = true;
      break;
    }
  }

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  Pds::Cphw::Reg::set(ip, 8192, 0);

  Module* m = new((void*)0x80000000) Module;

#ifdef USE_STREAM
  NetIODev  root = INetIODev::create("fpga", ip);

  {  //  Register access                                                        
    INetIODev::PortBuilder bldr = INetIODev::createPortBuilder();
    bldr->setSRPVersion              ( INetIODev::SRP_UDP_V3 );
    bldr->setUdpPort                 (                  8193 );
    bldr->setUdpOutQueueDepth        (                    40 );
    bldr->setUdpNumRxThreads         (                     4 );
    bldr->setDepackOutQueueDepth     (                     5 );
    bldr->setDepackLdFrameWinSize    (                     5 );
    bldr->setDepackLdFragWinSize     (                     5 );
    bldr->setSRPTimeoutUS            (                 90000 );
    bldr->setSRPRetryCount           (                     5 );
    bldr->setSRPMuxVirtualChannel    (                     0 );
    bldr->useDepack                  (                  true );
    bldr->useRssi                    (                  true );
    bldr->setTDestMuxTDEST           (                  0xc0 );

    Field f = IIntField::create("data");
    root->addAtAddress( f, bldr);
  }

  Path strmPath = root->findByName("data");
  Stream strm = IStream::create( strmPath );
#endif

  for(unsigned i=0; i<10; i++) {
    m->_analysisRst=0xffff;
    m->_analysisRst=0;

    timespec begin, end;
    clock_gettime(CLOCK_REALTIME,&begin);

#ifdef USE_STREAM
    CAxisFrameHeader hdr;
    uint8_t          buf[1500];
    hdr.insert(buf, sizeof(buf));
    hdr.iniTail(buf + hdr.getSize()+0x100);
    unsigned sz = hdr.getSize()+hdr.getTailSize()+0x100;
    uint32_t* bb = reinterpret_cast<uint32_t*>(&buf[hdr.getSize()]);
    for(unsigned j=0; j<10000; j++) {
      bb[j&0x3f] = (0xff0000|(j&0xff));
      if ((j&0x3f)==0x3f)
        strm->write( (uint8_t*)buf, sz);
    }
#else
    for(unsigned j=0; j<10000; j++)
      m->_analysisTag=(0xff0000|(j&0xff));
#endif

    clock_gettime(CLOCK_REALTIME,&end);

    unsigned n = m->_analysisTagWr;

    printf("analysisTag n %u  dt %f\n",
           n, dtime(end,begin));
  }

  return 0;
}
