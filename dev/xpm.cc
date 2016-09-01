
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
//#include "pds/xpm/ClockControl.hh"
//#include "pds/xpm/JitterCleaner.hh"
#include "pds/cphw/RingBuffer.hh"
#include "pds/cphw/AxiVersion.hh"

#include <string>

static const unsigned short port_req = 11000;

static inline double dtime(timespec& tsn, timespec& tso)
{
  return double(tsn.tv_sec-tso.tv_sec)+1.e-9*(double(tsn.tv_nsec)-double(tso.tv_nsec));
}

extern int optind;

using namespace Pds::Xpm;

void usage(const char* p) {
  printf("Usage: %s [options]\n",p);
  printf("Options: -a <IP addr (dotted notation)> : Use network <IP>\n");
  printf("         -F <fixed rate marker>         : Choose fixed rate trigger\n");
  printf("         -L <rx link>                   : Select rx link for full feedback\n");
  printf("         -D                             : Disable selected link\n");
  printf("         -R                             : Reset selected link\n");
  printf("         -r                             : Reset PLL\n");
  printf("         -s <skew steps>                : Change PLL skew\n");
  printf("         -b                             : Bypass PLL\n");
}

void sigHandler( int signal ) {
  Module* m = new((void*)0x80000000) Module;
  m->setL0Enabled(false);
  ::exit(signal);
}

static void* handle_req(void*);

int main(int argc, char** argv) {

  extern char* optarg;

  int c;
  bool lUsage = false;

  unsigned ip = 0xc0a8020a;
  unsigned short port = 8192;
  unsigned linkEnable   =0;
  unsigned linkLoopback =0;
  int fixedRate=-1;
  int skewSteps=0;
  bool lreset=false;
  bool lenable=true;
  bool lloopback=false;
  bool lPll=false;
  bool lPllbypass=false;
  int  ringChan=-1;
  int  freqSel=-1;
  int  bwSel=-1;

  while ( (c=getopt( argc, argv, "a:F:L:l:s:B:f:S:Rbrh")) != EOF ) {
    switch(c) {
    case 'a':
      ip = ntohl(inet_addr(optarg)); break;
      break;
    case 'F':
      fixedRate = atoi(optarg);
      break;
    case 'L':
      linkEnable = strtoul(optarg,NULL,0);
      break;
    case 'R':
      lreset = true;
      break;
    case 'f':
      freqSel = strtoul(optarg,NULL,0);
      break;
    case 'S':
      bwSel = strtoul(optarg,NULL,0);
      break;
    case 'B':
      ringChan = strtoul(optarg,NULL,0);
      break;
    case 'r':
      lPll = true;
      break;
    case 'b':
      lPllbypass = true;
      break;
    case 's':
      skewSteps = atoi(optarg);
      break;
    case 'l':
      linkLoopback = strtoul(optarg,NULL,0);
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

  Pds::Cphw::Reg::set(ip, port, 0);

  { Pds::Cphw::AxiVersion* vsn = new((void*)0) Pds::Cphw::AxiVersion;
    printf("buildStamp %s\n",vsn->buildStamp().c_str()); }

  Module*         m = new((void*)0x80000000) Module;
  //  ClockControl*  cc = new((void*)0x81000000) ClockControl;
  //  JitterCleaner* jc = new((void*)0x05000000) JitterCleaner;

  printf("pllStatus 0x%x:%x\n",
         unsigned(m->_pllStatus0),
         unsigned(m->_pllStatus1));
  printf("pllConfig 0x%x\n",
         unsigned(m->_pllConfig0));

  if (lPll) {
    m->pllBwSel  (7);
    m->pllRateSel(0xa);
    m->pllFrqSel (0x692);
    m->pllBypass (false);
    m->pllReset  ();
    usleep(10000);
    printf("pllStatus 0x%x:%x\n",
           unsigned(m->_pllStatus0),
           unsigned(m->_pllStatus1));
  }

  if (freqSel>=0) {
    m->pllFrqSel(freqSel);
    m->pllReset();
  }

  if (bwSel>=0) {
    m->pllBwSel(bwSel);
    m->pllReset();
  }

  if (skewSteps)
    m->pllSkew(skewSteps);

  if (lPllbypass) {
    m->pllBypass(true);
    m->pllReset();
  }

  for(unsigned i=0; linkLoopback; i++)
    if (linkLoopback&(1<<i)) {
      m->linkLoopback(i,true);
      linkLoopback &= ~(1<<i);
    }

  if (lreset) {
    unsigned linkReset=linkEnable;
    for(unsigned i=0; linkReset; i++) 
      if (linkReset&(1<<i)) {
        m->txLinkReset(i);
        m->rxLinkReset(i);
        usleep(10000);
        linkReset &= ~(1<<i);
      }
  }

  if (ringChan>=0) {
    m->setRingBChan(ringChan);

    Pds::Cphw::RingBuffer* b = new((void*)0x80010000) Pds::Cphw::RingBuffer;
    
    b->enable(false);
    b->clear();
    b->enable(true);
    usleep(1000);
    b->enable(false);
    b->dump();
  }

  m->setL0Enabled(false);
  if (fixedRate>=0)
    m->setL0Select_FixedRate(fixedRate);

  m->clearLinks();
  for(unsigned i=0; linkEnable; i++)
    if (linkEnable&(1<<i)) {
      m->linkEnable(i,true);
      linkEnable &= ~(1<<i);
    }
  L0Stats s = m->l0Stats();
  L0Stats s0 = s;
  m->setL0Enabled(true);
  
  m->init();
  printf("rx/tx Status: %08x/%08x\n", m->rxLinkStat(), m->txLinkStat());

  ::signal( SIGINT, sigHandler );

  //
  //  Create thread to receive analysis tag requests
  //
  { 
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_t tid;
    if (pthread_create(&tid, &tattr, &handle_req, 0))
      perror("Error creating tag request thread");
  }
  
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

void* handle_req(void* arg)
{
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd<0) { 
    perror("Error opening analysis tag request socket");
    return 0;
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr=0;
  addr.sin_port=htons(port_req);

  if (::bind(fd, (sockaddr*)&addr, sizeof(addr))) {
    perror("bind error");
    return 0;
  }

  char request[32];
  Module* m = new((void*)0x80000000) Module;

  m->_analysisRst = 0xffff;
  m->_analysisRst = 0;
  
  uint32_t otag=0;
  while( recv(fd, request, 32, 0)==4 ) {
    uint32_t tag = *reinterpret_cast<uint32_t*>(request);
    m->_analysisTag = tag;
    //    unsigned v = m->_analysisTag;
    //    printf("Requesting tag: %08x [%08x]\n",tag,v);
    if (otag && tag!=otag+1)
      printf("Requesting tag: %08x [%08x]\n",tag,otag);
    otag = tag;
  }

  printf("Done handling requests\n");
  return 0;
}
  
  
