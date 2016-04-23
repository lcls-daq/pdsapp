//
//  Run LCLS-II EVR in DAQ mode
//    L0T from partition word
//    No BSA
//    N trigger channels
//

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#include "evgr/evr/evr.hh"
#include "pds/service/CmdLineTools.hh"
#include "tpr.hh"

#include <string>

extern int optind;

static const unsigned NCHANNELS = 14;
static const unsigned NTRIGGERS = 12;
using namespace Pds;

class DaqStats {
public:
  DaqStats() : eventFrames(0),
               dropFrames (0),
               tagMisses  (0) {}
  DaqStats(const DaqStats& o) {
    clock_gettime(CLOCK_REALTIME,&tv);
    eventFrames = o.eventFrames;
    dropFrames  = o.dropFrames;
    tagMisses   = o.tagMisses;
  }
public:
  DaqStats& operator=(const DaqStats& o) {
    tv = o.tv;
    eventFrames = o.eventFrames;
    dropFrames  = o.dropFrames;
    tagMisses   = o.tagMisses;
    return *this;
  }
public:
  void dump(const DaqStats& o) {
    double dt = double(o.tv.tv_sec-tv.tv_sec)+1.e-9*(double(o.tv.tv_nsec)-double(tv.tv_nsec));
#define PSTAT(r) printf("%9u %12.12s [%u] : %g\n",r,#r,o.r-r,double(o.r-r)/dt)
    printf("-------------\n");
    PSTAT(eventFrames);
    PSTAT(dropFrames);
    PSTAT(tagMisses);
  }
public:
  struct timespec tv;
  unsigned eventFrames;
  unsigned dropFrames;
  unsigned tagMisses;
};

static DaqStats daqStats;

static unsigned nPrint = 20;

static void* read_thread(void*);

// Memory map of TPR registers (EvrCardG2 BAR 1)
class TprReg {
public:
  TprBase  base;
  volatile uint32_t reserved_0    [(0x400-sizeof(TprBase))/4];
  DmaControl dma;
  volatile uint32_t reserved_1    [(0x10000-0x400-sizeof(DmaControl))/4];
  XBar     xbar;
  volatile uint32_t reserved_xbar [(0x30000-sizeof(XBar))/4];
  TprCore  tpr;
  volatile uint32_t reserved_tpr  [(0x10000-sizeof(TprCore))/4];
  RingB    ring0;
  volatile uint32_t reserved_ring0[(0x10000-sizeof(RingB))/4];
  RingB    ring1;
  volatile uint32_t reserved_ring1[(0x10000-sizeof(RingB))/4];
  TpgMini  tpg;
};

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b> [-T|-L]\n",p);
  printf("\t-L program xbar for nearend loopback\n");
  printf("\t-T accesses TPR (BAR1)\n");
}

int main(int argc, char** argv) {

  extern char* optarg;
  char evrid='a';
  unsigned channels=1;
  unsigned partition=0;

  char* endptr;
  int c;
  bool lUsage = false;
  bool parseOK;
  while ( (c=getopt( argc, argv, "r:C:v:h")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'C':
      channels = strtoul(optarg,NULL,0);
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

  //
  //  Configure the XBAR for straight-in/out
  //
  {
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

    p->xbar.setTpr(XBar::StraightIn);
    p->xbar.setTpr(XBar::StraightOut);
  }

  //
  //  Configure channels, event selection
  //
    char evrdev[16];
    sprintf(evrdev,"/dev/er%c3_1",evrid);
    printf("Using tpr %s\n",evrdev);

    int fd = open(evrdev, O_RDWR);
    if (fd<0) {
      perror("Could not open");
      return -1;
    }

    void* ptr = mmap(0, sizeof(TprReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      perror("Failed to map");
      return -2;
    }

    TprReg* p = reinterpret_cast<TprReg*>(ptr);

    p->tpr.rxPolarity(false);
    p->tpr.resetCounts(); 

    p->base.dump();
    p->dma.dump();
    p->tpr.dump();

    //
    //  Create thread to receive DMAS and validate the data
    //
    { 
      pthread_attr_t tattr;
      pthread_attr_init(&tattr);
      pthread_t tid;
      if (pthread_create(&tid, &tattr, &read_thread, &fd))
        perror("Error creating read thread");
    }
     
    p->base.countReset = 1;
    usleep(1);
    p->base.countReset = 0;

    for(unsigned i=0; i<32; i++)
      if (channels&(1<<i))
        p->base.setupDaq(i,partition);

    p->base.dump();


  DaqStats ostats(daqStats);
  unsigned och0=0;

  while(1) {
    usleep(1000000);

    unsigned dmaStat = p->dma.rxFreeStat;
    printf("Full/Valid/Empty : %u/%u/%u  Count :%x\n",
           (dmaStat>>31)&1, (dmaStat>>30)&1, (dmaStat>>29)&1,
           dmaStat&0x3ff);

    DaqStats stats(daqStats);
    ostats.dump(stats);
    ostats = stats;

    printf("RxErrs/Resets: %08x/%08x\n", 
           p->tpr.RxDecErrs+p->tpr.RxDspErrs,
           p->tpr.RxRstDone);

    { unsigned uch0 = p->base.channel[0].evtCount;
      unsigned utot = p->base.channel[6].evtCount;
      printf("eventCount: %08x:%08x [%d]\n",uch0,utot,uch0-och0);
      och0 = uch0;
    }
  }

  return 0;
}

void* read_thread(void* arg)
{
  int fd = *reinterpret_cast<int*>(arg);

  uint32_t* data = new uint32_t[1024];
  
  EvrRxDesc* desc = new EvrRxDesc;
  desc->maxSize = 1024;
  desc->data    = data;
  
  unsigned ntag=0;

  ssize_t nb;
  while(1) {
    if ((nb = read(fd, desc, sizeof(*desc)))>=0) {
      uint64_t* p     = (uint64_t*)data;
      if (((p[0]>>32)&0xffff)==0) {
        daqStats.eventFrames++;
        uint16_t* pword = (uint16_t*)data+42;
        ntag = ((pword[0]>>1)+1)&0x1f;
        break;
      }
    }
  }

  uint64_t opid = 0;

  while ((nb = read(fd, desc, sizeof(*desc)))>=0) {
    uint64_t* p     = (uint64_t*)data;
    if (p[0]&(1ULL<<30)) {
      daqStats.dropFrames++;
    }

    if (((p[0]>>32)&0xffff)==0) {
      daqStats.eventFrames++;

      uint16_t* pword = (uint16_t*)data+49;
      unsigned tag = (pword[0]>>1)&0x1f;

      if (tag != ntag) {
        printf("Tag miss: %x:%x  pid: %016llx:%016llx\n",
               tag, ntag, p[2], opid);
      }
      opid = p[2];

      daqStats.tagMisses += ((tag-ntag)&0x1f);
      ntag = (tag+1)&0x1f;

      if (nPrint) {
        nPrint--;
	printf("EVENT  [0x%x]:",pword[0]);
        for(unsigned i=0; i<15; i++)
          printf(" %16llx",p[i]);
        printf("\n");
      }
    }
  }

  return 0;
}
