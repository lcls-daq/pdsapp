
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#include "pds/tpr/Module.hh"
#include "evgr/evr/evr.hh"

#include <string>

extern int optind;

static const unsigned NCHANNELS = 14;
static const unsigned NTRIGGERS = 12;

static unsigned eventFrames;
static unsigned bsaControlFrames;
static unsigned bsaChannelFrames[NCHANNELS];
static unsigned bsaChannelAcqs  [NCHANNELS];
static uint64_t bsaChannelMask  [NCHANNELS];
static unsigned dropFrames;
static unsigned nPrint = 0;

static void* read_thread(void*);

using namespace Pds::Tpr;

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b> [-T|-L]\n",p);
  printf("\t-L program xbar for nearend loopback\n");
  printf("\t-T accesses TPR (BAR1)\n");
}

int main(int argc, char** argv) {

  extern char* optarg;
  char evrid='a';
  bool lTpr=false;
  bool lTpg=false;
  bool lReset=false;
  bool lPolarity=false;
  bool lLbNE=false;
  bool lLbFE=false;
  bool lDumpRing=false;
  bool lBsaTest=false;
  unsigned bsaDelay=0,bsaWidth=0;
  bool lNoOp=false;
  bool lTrigger=false;
  unsigned trigDelay=0,trigWidth=0,trigPolarity=0;

  char* endptr;
  int c;
  bool lUsage = false;
  while ( (c=getopt( argc, argv, "r:hF:GLOTPRDB:0v:")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'L':
      lLbNE=true;
      break;
    case 'O':
      lLbFE=true;
      break;
    case 'T':
      lTpr = true;
      break;
    case 'P':
      lPolarity = true;
      break;
    case 'R':
      lReset = true;
      break;
    case 'G':
      lTpg = true;
      break;
    case 'D':
      lDumpRing = true;
      break;
    case 'B':
      bsaDelay = strtoul(optarg,&endptr,0);
      bsaWidth = strtoul(endptr+1,&endptr,0);
      lBsaTest = true;
      break;
    case 'F':
      trigDelay = strtoul(optarg,&endptr,0);
      trigWidth = strtoul(endptr+1,&endptr,0);
      trigPolarity = strtoul(endptr+1,&endptr,0);
      lTrigger = true;
      break;
    case '0':
      lNoOp = true;
      break;
    case 'v':
      nPrint = strtoul(optarg,NULL,0);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    case '?':
    default:
      lUsage = true;
      break;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  if (!lTpr) {
    
    char evrdev[16];
    sprintf(evrdev,"/dev/er%c3",evrid);
    printf("Using evr %s\n",evrdev);

    int fd = open(evrdev, O_RDWR);
    if (fd<0) {
      perror("Could not open");
      return -1;
    }

    void* ptr = mmap(0, sizeof(EvrReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      perror("Failed to map");
      return -2;
    }

    EvrReg* p = reinterpret_cast<EvrReg*>(ptr);

    printf("SLAC Version[%p]: %08x\n", 
	   &(p->evr),
	   ((volatile uint32_t*)(&p->evr))[0x30>>2]);

    p->evr.IrqEnable(0);

    printf("Axi Version [%p]: BuildStamp: %s\n", 
	   &(p->version),
	   p->version.buildStamp().c_str());
    
    printf("[%p] [%p] [%p]\n",p, &(p->version), &(p->xbar));

    p->xbar.dump();

    if (lLbNE)
      p->xbar.setTpr(XBar::LoopIn);
    else 
      p->xbar.setTpr(XBar::StraightIn);
    if (lLbFE)
      p->xbar.setTpr(XBar::LoopOut);
    else
      p->xbar.setTpr(XBar::StraightOut);

    p->xbar.dump();

    if (lTrigger) {
    }
  }
  else {
    char evrdev[16];
    sprintf(evrdev,"/dev/er%c3_1",evrid);
    printf("Using tpr %s\n",evrdev);

    int fd = open(evrdev, O_RDWR);
    if (fd<0) {
      perror("Could not open");
      return -1;
    }

    if (lNoOp)
      return 1;

    { printf("sizeof(TprReg): %08x\n", unsigned(sizeof(TprReg)));
      TprReg* r = 0;
      printf("xbar  [%p]\n",&r->xbar);
      printf("tpr   [%p]\n",&r->tpr);
      printf("dma   [%p]\n",&r->dma);
      printf("ring0 [%p]\n",&r->ring0);
      printf("ring1 [%p]\n",&r->ring1);
      printf("tpg   [%p]\n",&r->tpg); }

    void* ptr = mmap(0, sizeof(TprReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      perror("Failed to map");
      return -2;
    }

    TprReg* p = reinterpret_cast<TprReg*>(ptr);
    printf("tpr[%p]:tpg[%p]\n",&p->tpr,&p->tpg);
    p->base.dump();
    p->dma.dump();
    //    p->dma.test();
    p->tpr.dump();
    if (lReset) {
      p->tpr.rxPolarity (lPolarity);
      p->tpr.resetCounts(); 
    }
    if (lTpg) {
      p->tpg.dump();
      if (lDumpRing) {
	p->ring0.enable(false);
	p->ring0.clear();
	p->ring0.enable(true);
	usleep(1000);
	p->ring0.enable(false);
	p->ring0.dump();
	//p->ring0.dumpFrames();
      }
    }
    else if (lBsaTest) {
      { 
	pthread_attr_t tattr;
	pthread_attr_init(&tattr);
	pthread_t tid;
	if (pthread_create(&tid, &tattr, &read_thread, &fd))
	  perror("Error creating read thread");
      }
      p->base.countReset = 1;
      { timespec ts; ts.tv_sec=0; ts.tv_nsec=1000;
	nanosleep(&ts,0); }
      p->base.countReset = 0;
      p->base.setupChannel(0, TprBase::Any, TprBase::_100K,
                           0, bsaDelay, bsaWidth);
      p->ring0.enable(false);
      p->ring0.clear();
      p->ring0.enable(true);

      p->tpg.BsaDef[0].l = (1<<31) | (0); // 100kHz any destination
      p->tpg.BsaDef[0].h = (5000<<16) | (1000); // 100 acquisitions

      timespec ts; ts.tv_sec=0; ts.tv_nsec=100000;
      nanosleep(&ts,0);

      p->ring0.enable(false);
      p->ring0.dumpFrames();

      usleep(10000000);
      
      p->base.dump();
      p->base.channel[0].control = 0;
      
#define print_u64(q) {					\
	uint64_t v  = uint64_t(q);			\
	v = v | (uint64_t(q)<<32);			\
	printf("%16lx ",v);	\
      }

      printf("BSA Control Data\n");
      for(unsigned i=0; i<4; i++) {
	print_u64(p->base.bsaCntlData);
	print_u64(p->base.bsaCntlData);
	print_u64(p->base.bsaCntlData);
	printf("\n");
      }

      printf("BSA Channel Data\n");
      for(unsigned i=0; i<40; i++) {
	print_u64(p->base.channel[0].bsaData);
	print_u64(p->base.channel[0].bsaData);
	print_u64(p->base.channel[0].bsaData);
	printf("\n");
      }

      sleep(5);

      printf("EventFrames %u\n", eventFrames);
      printf("DropFrames  %u\n", dropFrames);
      printf("BsaControlF %u\n", bsaControlFrames);
      for(unsigned i=0; i<NCHANNELS; i++)
	printf("BsaChF[%u] %u %u %16zx\n", 
	       i, bsaChannelFrames[i], bsaChannelAcqs[i], bsaChannelMask[i]);

#undef print_u64
    }
    else if (lTrigger) {
      if (trigWidth==0) {
        p->base.trigger[0].control=0;
        p->base.trigMaster=0;
      }
      else {
        p->base.trigMaster=1;
        p->base.setupChannel(1, TprBase::Any, TprBase::_1K,
                             0, bsaDelay, bsaWidth);
        p->base.channel[1].control=1;
        p->base.setupTrigger(0,1,trigPolarity,trigDelay,trigWidth);
        p->base.dump();
      }
    }
  }

  return 0;
}

void* read_thread(void* arg)
{
  int fd = *reinterpret_cast<int*>(arg);

  uint32_t* data = new uint32_t[1024];
  
  Pds::Tpr::RxDesc* desc = new Pds::Tpr::RxDesc(data,1024);

  eventFrames = 0;
  bsaControlFrames = 0;
  memset(bsaChannelFrames,0,NCHANNELS*sizeof(unsigned));
  memset(bsaChannelAcqs  ,0,NCHANNELS*sizeof(unsigned));
  memset(bsaChannelMask  ,0,NCHANNELS*sizeof(unsigned));
  dropFrames = 0;

  ssize_t nb;
  while( (nb = read(fd, desc, sizeof(*desc))) >= 0 ) {
    uint64_t* p = (uint64_t*)data;
    
    char m = ' ';
    if (p[0]&(1ULL<<30)) {
      m = 'D';
      dropFrames++;
    }

    switch((p[0]>>32)&0xffff) {
    case 0:
      eventFrames++;
      if (nPrint)
	printf("EVENT  [x%lx]: %16lx %16lx %16lx %16lx %c\n",
	       (p[0]>>48)&0xffff,p[1],p[2],p[3],p[4],m);
      break;
    case 1:
      bsaControlFrames++;
      if (nPrint)
	printf("BSA_CNTL: %16lx %16lx %16lx %c\n",
	       p[1],p[2],p[3],m);
      break;
    case 2:
      bsaChannelFrames[(p[0]>>48)&0xf]++;
      bsaChannelMask  [(p[0]>>48)&0xf] |= p[2];
      if (p[2]!=0)
	bsaChannelAcqs[(p[0]>>48)&0xf]++;

      if (nPrint)
	printf("BSA_CHAN[%lu]: %16lx %16lx %16lx %c\n",
	       (p[0]>>48)&0xff,p[1],p[2],p[3],m);
      break;
    default:
      break;
    }

    if (nPrint) nPrint--;
  }

  return 0;
}
