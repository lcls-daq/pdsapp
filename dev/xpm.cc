
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

#include "pds/xpm/Module.hh"
#include "pds/xpm/RingBuffer.hh"

#include <string>

static inline double dtime(timespec& tsn, timespec& tso)
{
  return double(tsn.tv_sec-tso.tv_sec)+1.e-9*(double(tsn.tv_nsec)-double(tso.tv_nsec));
}

extern int optind;

using namespace Xpm;

void usage(const char* p) {
  printf("Usage: %s [-a <IP addr (dotted notation)>]\n",p);
}

void sigHandler( int signal ) {
  Xpm::Module* m = new(0) Xpm::Module;
  m->setL0Enabled(false);
  ::exit(signal);
}

int main(int argc, char** argv) {

  extern char* optarg;

  int c;
  bool lUsage = false;

  unsigned ip = 0xc0a8020a;
  unsigned short port = 8192;
  int fixedRate=-1;
  bool lreset=false;
  bool lenable=true;

  while ( (c=getopt( argc, argv, "a:F:RDh")) != EOF ) {
    switch(c) {
    case 'a':
      ip = ntohl(inet_addr(optarg)); break;
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

  Reg::set(ip, port, 0x80000000);

  Xpm::Module* m = new(0) Xpm::Module;

  if (lreset) 
    m->rxLinkReset(0);

#if 1
  Xpm::RingBuffer* b = new((void*)0x10000) Xpm::RingBuffer;

  b->enable(false);
  b->clear();
  b->enable(true);
  usleep(1000);
  b->enable(false);
  b->dump();
#endif

  m->init();
  printf("rx/tx Status: %08x/%08x\n", m->rxLinkStat(), m->txLinkStat());

  m->setL0Enabled(false);
  if (fixedRate>=0)
    m->setL0Select_FixedRate(fixedRate);
  L0Stats s = m->l0Stats();
  L0Stats s0 = s;
  m->linkEnable(0,lenable);
  m->setL0Enabled(true);
  
  ::signal( SIGINT, sigHandler );

  while(1) {
    usleep(1000000);
    L0Stats n = m->l0Stats();

#define DSTAT(name,val) {                                             \
      printf("%10.10s  : %16lx [%lu]\n", #name,                       \
             (n.val-s0.val),                                          \
             (n.val-s.val)); }

    printf("-------\n");
    DSTAT(Enabled  ,l0Enabled);
    DSTAT(Inhibited,l0Inhibited);
    DSTAT(Num      ,numl0);
    DSTAT(NumInh   ,numl0Inh);
    DSTAT(NumAcc   ,numl0Acc);
    printf("%10.10s  : %04x [%u]\n", "Rx0Errs",
             (unsigned)(n.rx0Errs-s0.rx0Errs)&0x3fff,
             (unsigned)(n.rx0Errs-s.rx0Errs)&0x3fff);
#undef DSTAT

    s = n;
  }

  return 0;
}
