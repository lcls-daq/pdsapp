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
#include <semaphore.h>
#include <poll.h>
#include <signal.h>
#include <arpa/inet.h>

#include "evgr/evr/evr.hh"

#include <string>
#include <vector>

#include "pds/service/Histogram.hh"
#include "pds/tprds/Module.hh"

using namespace Pds::TprDS;
using Pds::Tpr::EvrReg;
using Pds::Tpr::XBar;
using Pds::Tpr::RxDesc;

extern int optind;

static const unsigned NCHANNELS = 14;
static const unsigned NTRIGGERS = 12;
static const unsigned short port_req = 11000;

class ThreadArgs {
public:
  int fd;
  unsigned busyTime;
  sem_t sem;
  int reqfd;
};


class DaqStats {
public:
  DaqStats() : _values(7) {
    for(unsigned i=0; i<_values.size(); i++)
      _values[i]=0;
  }
public:
  static const char** names();
  std::vector<unsigned> values() const { return _values; }
public:
  unsigned& eventFrames () { return _values[0]; }
  unsigned& dropFrames  () { return _values[1]; }
  unsigned& repeatFrames() { return _values[2]; }
  unsigned& tagMisses   () { return _values[3]; }
  unsigned& corrupt     () { return _values[4]; }
  unsigned& anaTags     () { return _values[5]; }
  unsigned& anaErrs     () { return _values[6]; }
private:
  std::vector<unsigned> _values;
};  

const char** DaqStats::names() {
  static const char* _names[] = {"eventFrames",
                                 "dropFrames",
                                 "repeatFrames",
                                 "tagMisses",
                                 "corrupt",
                                 "anaTags",
                                 "anaErrs" };
  return _names;
}


class DmaStats {
public:
  DmaStats() : _values(4) {
    for(unsigned i=0; i<_values.size(); i++)
      _values[i]=0;
  }
  DmaStats(TprBase& o) : _values(4) {
    //    frameCount   () = o.frameCount;
    frameCount   () = o.channel[1].evtCount;
    pauseCount   () = o.pauseCount;
    overflowCount() = o.overflowCount;
    idleCount    () = o.idleCount;
  }

public:
  static const char** names();
  std::vector<unsigned> values() const { return _values; }
public:
  unsigned& frameCount   () { return _values[0]; }
  unsigned& pauseCount   () { return _values[1]; }
  unsigned& overflowCount() { return _values[2]; }
  unsigned& idleCount    () { return _values[3]; }
private:
  std::vector<unsigned> _values;
};  

const char** DmaStats::names() {
  static const char* _names[] = {"frameCount",
                                 "pauseCount",
                                 "overflowCount",
                                 "idleCount" };
  return _names;
}


template <class T> class RateMonitor {
public:
  RateMonitor() {}
  RateMonitor(const T& o) {
    clock_gettime(CLOCK_REALTIME,&tv);
    _t = o;
  }
  RateMonitor<T>& operator=(const RateMonitor<T>& o) {
    tv = o.tv;
    _t = o._t;
    return *this;
  }
public:
  void dump(const RateMonitor<T>& o) {
    double dt = double(o.tv.tv_sec-tv.tv_sec)+1.e-9*(double(o.tv.tv_nsec)-double(tv.tv_nsec));
    for(unsigned i=0; i<_t.values().size(); i++)
      printf("%10u %15.15s [%10u] : %g\n",
             _t.values()[i],
             _t.names()[i],
             o._t.values()[i]-_t.values()[i],
             double(o._t.values()[i]-_t.values()[i])/dt);
  }
private:
  timespec tv;
  T _t;
};
  

static DaqStats  daqStats;
static Pds::Histogram readSize(6,1);

static unsigned nPrint = 20;

static void* read_thread(void*);

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b> [-T|-L]\n",p);
  printf("\t-L program xbar for nearend loopback\n");
  printf("\t-T accesses TPR (BAR1)\n");
}

static TprReg* tprReg=0;
static int partition = -1;

void sigHandler( int signal ) {
  if (tprReg) {
    tprReg->base.channel[0].control=0;
  }
  readSize.dump();

  ::exit(signal);
}

int main(int argc, char** argv) {
  extern char* optarg;
  char evrid='a';
  unsigned emptyThr=2;
  unsigned fullThr=-1U;
  unsigned length=16;  // transfer is +6 words (up to 28)
  unsigned rate=6;
  const char* addr_req = 0;
  ThreadArgs args;
  args.fd = -1;
  args.busyTime = 0;
  
  int c;
  bool lUsage = false;
  while ( (c=getopt( argc, argv, "r:v:S:B:E:F:R:P:T:h")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'B':
      args.busyTime = strtoul(optarg,NULL,0);
      break;
    case 'E':
      emptyThr = strtoul(optarg,NULL,0);
      break;
    case 'F':
      fullThr = strtoul(optarg,NULL,0);
      break;
    case 'S':
      length = strtoul(optarg,NULL,0);
      break;
    case 'R':
      rate = strtoul(optarg,NULL,0);
      break;
    case 'P':
      partition = strtoul(optarg,NULL,0);
      break;
    case 'T':
      addr_req = optarg;
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

    void* ptr = mmap(0, sizeof(Pds::Tpr::EvrReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
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
    //p->xbar.setTpr(XBar::LoopIn);
    p->xbar.setTpr(XBar::StraightOut);
  }

  //
  //  Setup request pipeline
  //
  if (addr_req) {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd<0) { 
      perror("Error opening analysis tag request socket");
      return 0;
    }
    
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr=inet_addr(addr_req);
    addr.sin_port=htons(port_req);

    connect(fd, (sockaddr*)&addr, sizeof(addr));

    args.reqfd = fd;
    printf("Opened socket %d to %s\n",fd, addr_req);
  }
  else
    args.reqfd = -1;

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

    args.fd  = fd;
    sem_init(&args.sem,0,0);

    void* ptr = mmap(0, sizeof(TprReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      perror("Failed to map");
      return -2;
    }

    TprReg* p = tprReg = reinterpret_cast<TprReg*>(ptr);

    p->tpr.rxPolarity(false);  // RTM
    //    p->tpr.rxPolarity(true); // Timing RTM
    p->tpr.resetCounts(); 
    p->dma.setEmptyThr(emptyThr);
    p->base.dmaFullThr=fullThr;

    p->base.dump();
    p->dma.dump();
    p->tpr.dump();
    
    //  flush out all the old
    { printf("flushing\n");
      unsigned nflush=0;
      uint32_t* data = new uint32_t[1024];
      RxDesc* desc = new RxDesc(data,1024);
      pollfd pfd;
      pfd.fd = args.fd;
      pfd.events = POLLIN;
      while(poll(&pfd,1,0)>0) { 
        read(args.fd, desc, sizeof(*desc));
        nflush++;
      }
      delete[] data;
      delete desc;
      printf("done flushing [%u]\n",nflush);
    }
    
    //
    //  Create thread to receive DMAS and validate the data
    //
    { 
      pthread_attr_t tattr;
      pthread_attr_init(&tattr);
      pthread_t tid;
      if (pthread_create(&tid, &tattr, &read_thread, &args))
        perror("Error creating read thread");
      usleep(10000);
    }

    ::signal( SIGINT, sigHandler );

    //    sem_wait(&args.sem);
    p->base.countReset = 1;
    usleep(1);
    p->base.countReset = 0;

    //  only one channel implemented
    if (partition>=0)
      p->base.setupDaq (0,partition,length);
    else
      p->base.setupRate(0,rate,length);

    p->base.dump();

  RateMonitor<DaqStats> ostats(daqStats);
  DmaStats d;
  RateMonitor<DmaStats> dstats(d);

  unsigned och0=0;
  unsigned otot=0;

  while(1) {
    usleep(1000000);

    printf("--------------\n");
    unsigned dmaStat = p->dma.rxFreeStat;
    printf("Full/Valid/Empty : %u/%u/%u  Count :%x\n",
           (dmaStat>>31)&1, (dmaStat>>30)&1, (dmaStat>>29)&1,
           dmaStat&0x3ff);

    { RateMonitor<DaqStats> stats(daqStats);
      ostats.dump(stats);
      ostats = stats; }

    printf("RxErrs/Resets: %08x/%08x\n", 
           p->tpr.RxDecErrs+p->tpr.RxDspErrs,
           p->tpr.RxRstDone);

    { unsigned uch0 = p->base.channel[ 0].evtCount;
      unsigned utot = p->base.channel[12].evtCount;
      printf("eventCount: %08x:%08x [%d:%d]\n",uch0,utot,uch0-och0,utot-otot);
      och0 = uch0;
      otot = utot;
    }

    { DmaStats d(p->base);
      RateMonitor<DmaStats> dmaStats(d);
      dstats.dump(dmaStats);
      dstats = dmaStats; }
  }

  return 0;
}

void* read_thread(void* arg)
{
  ThreadArgs targs = *reinterpret_cast<ThreadArgs*>(arg);

  uint32_t* data = new uint32_t[1024];
  
  RxDesc* desc = new RxDesc(data,1024);
  
  unsigned ntag = 0;
  uint64_t opid = 0;
  unsigned anaw = 0;
  unsigned anar = -1;
  ssize_t nb;

  //  sem_post(&targs.sem);

  while(1) {
    if ((nb = read(targs.fd, desc, sizeof(*desc)))>=0) {
      { printf("READ %zd words\n",nb);
        uint32_t* p     = (uint32_t*)data;
        for(unsigned i=0; i<nb; i++)
          printf(" %08x",p[i]);
        printf("\n"); }
      uint32_t* p     = (uint32_t*)data;
      if (((p[1]>>0)&0xffff)==0) {
        daqStats.eventFrames()++;
        ntag = ((p[2]>>1)+1)&0x1f;
        opid = p[4];
        opid = (opid<<32) | p[3];
        break;
      }
    }
  }

  uint64_t pid_busy = opid+(1ULL<<20);

  if (targs.reqfd>=0) {
    for(unsigned i=0; i<128; i++) {
      unsigned v = anaw | (0xffff<<16);
      send(targs.reqfd, &v, sizeof(v), 0);
      anaw++;
    }
  }

  while (1) {
    if ((nb = read(targs.fd, desc, sizeof(*desc)))<0)
      break;

    readSize.bump(nb);

    uint32_t* p     = (uint32_t*)data;
    uint32_t  len   = p[0];
    uint32_t  etag  = p[1];
    uint32_t  pword = p[2];
    uint64_t  pid   = p[4]; pid = (pid<<32)|p[3];

    if (etag&(1ULL<<30)) {
      daqStats.dropFrames()++;
    }

    if ((etag&0xffff)==0) {
      daqStats.eventFrames()++;

      unsigned tag = (pword>>1)&0x1f;

      if (partition>=0 && tag != ntag) {
        printf("Tag error: %x:%x  pid: %016lx:%016lx\n",
               tag, ntag, pid, opid);
        if (pid==opid) {
          daqStats.repeatFrames()++;
        }
        else {
          //          if ((p[2]>>48)==1) {
          if (0) {
            daqStats.corrupt()++;
            printf("corrupt [%zd]: ",nb);
            uint32_t* p32 = (uint32_t*)data;
            for(unsigned i=0; i<15; i++)
              printf(" %08x",p32[i]);
            printf("\n"); 
          }
          else
            daqStats.tagMisses() += ((tag-ntag)&0x1f);
        }
      }

      if (targs.reqfd>=0) {
        unsigned anatag = pword>>16;
        if (anar == -1 && anatag == 0xffff)
          printf("Tag %04x\n",anatag);
        else if (anatag != ((anar+1)&0xffff)) {
          printf("Tag %04x [%04x] [%04x]\n",anatag,(anar+1)&0xffff, anaw);
          daqStats.anaErrs()++;
          if (daqStats.anaErrs()>256)
            exit(1);
        }
        else {
          daqStats.anaTags()++;
          anar++;
        }

        // Test what happens when the analysis tag FIFO underruns
        //        if ((tag&0x1f)==0)
        //          ;
        //        else {
        {
          unsigned v = anaw | (0xffff<<16);
          send(targs.reqfd, &v, sizeof(v), 0);
          anaw++;
        }
      }

      opid = pid;

      ntag = (tag+1)&0x1f;

      if (nPrint) {
        nPrint--;
	printf("EVENT  [0x%x]:",(etag&0xffff));
        for(unsigned i=0; i<nb+1; i++)
          printf(" %08x",p[i]);
        printf("\n");
      }

      if (targs.busyTime && opid > pid_busy) {
        usleep(targs.busyTime);
        pid_busy = opid + (1ULL<<20);
      }
    }
  }

  printf("read_thread done\n");

  return 0;
}
