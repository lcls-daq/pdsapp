//
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

#include <string>
#include <vector>

#include "pds/service/Histogram.hh"
#include "pds/quadadc/Module.hh"

using namespace Pds::QuadAdc;

using Pds::TprDS::TprBase;
using Pds::Tpr::RxDesc;

extern int optind;

static const unsigned NCHANNELS = 14;
static const unsigned NTRIGGERS = 12;
static const unsigned short port_req = 11000;
static bool lVerbose = false;

class ThreadArgs {
public:
  int fd;
  unsigned busyTime;
  sem_t sem;
  int reqfd;
  int rate;
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
  DmaStats(QABase& o) : _values(4) {
    frameCount   () = o.countEnable;
    pauseCount   () = o.countInhibit;
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
static Pds::Histogram readSize(8,1);
static uint64_t opid = 0;
static Module::TestPattern pattern = Module::Flash11;
static QABase::Interleave qI=QABase::Q_NONE;

static unsigned nPrint = 20;

static void* read_thread(void*);
static bool checkFlash11_interleaved(uint32_t* p);
static bool checkFlash11(uint32_t* p);

void usage(const char* p) {
  printf("Usage: %s [options]\n",p);
  printf("Options:\n");
  printf("\t-I interleave\n");
  printf("\t-B busyTime : Sleeps for busytime seconds each read\n");
  printf("\t-E emptyThr : Set empty threshold for flow control\n");
  printf("\t-B fullThr  : \n");
  printf("\t-R rate     : Set trigger rate [0:929kHz, 1:71kHz, 2:10kHz, 3:1kHz, 4:100Hz, 5:10Hz\n");
  printf("\t-P partition: Set trigger source to partition\n");
  printf("\t-v nPrint   : Set number of events to dump out");
  printf("\t-V          : Dump out all events");
}

static Module* reg=0;
static int partition = -1;

void sigHandler( int signal ) {
  if (reg) {
    reg->base.stop();
    reg->dma_core.dump();
  }
  readSize.dump();
  printf("Last pid: %016lx\n",opid);

  ::exit(signal);
}

int main(int argc, char** argv) {
  extern char* optarg;
  char evrid='a';
  unsigned emptyThr=2;
  unsigned fullThr=-1U;
  unsigned length=16;  // multiple of 16
  ThreadArgs args;
  args.fd = -1;
  args.busyTime = 0;
  args.reqfd = -1;
  args.rate = 6;

  int c;
  bool lUsage = false;
  while ( (c=getopt( argc, argv, "Ir:v:S:B:E:F:R:P:T:Vh")) != EOF ) {
    switch(c) {
    case 'I':
      qI=QABase::Q_ABCD;
      break;
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
      args.rate = strtoul(optarg,NULL,0);
      break;
    case 'P':
      partition = strtoul(optarg,NULL,0);
      break;
    case 'T':
      pattern = (Module::TestPattern)strtoul(optarg,NULL,0);
      break;
    case 'v':
      nPrint = strtoul(optarg,NULL,0);
      break;
    case 'V':
      lVerbose = true;
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
  //  Configure channels, event selection
  //
  char dev[16];
  sprintf(dev,"/dev/qadc%c",evrid);
  printf("Using %s\n",dev);

  int fd = open(dev, O_RDWR);
  if (fd<0) {
    perror("Could not open");
    return -1;
  }

  args.fd  = fd;
  sem_init(&args.sem,0,0);

  void* ptr = mmap(0, sizeof(Module), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Failed to map");
    return -2;
  }

  Module* p = reg = reinterpret_cast<Module*>(ptr);

  p->i2c_sw_control.select(I2cSwitch::PrimaryFmc); 
  p->disable_test_pattern();
  p->enable_test_pattern(pattern);
  //  p->enable_test_pattern(Module::DMA);

  p->base.init();

  //  p->dma_core.init(0x400);
  p->dma_core.init(32+48*(length));
  // p->dma_core.init();
  p->dma_core.dump();

  //  p->dma.setEmptyThr(emptyThr);
  p->base.dmaFullThr=fullThr;

  p->base.dump();
  //  p->dma.dump();
  p->tpr.dump();

  //  flush out all the old
  { printf("flushing\n");
    unsigned nflush=0;
    uint32_t* data = new uint32_t[1<<20];
    RxDesc* desc = new RxDesc(data,1<<20);
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
  p->base.resetCounts();

  p->base.setChannels(0xf);
  p->base.setMode    (qI);

  if (partition>=0)
    p->base.setupDaq (partition,length);
  else
    p->base.setupLCLS(args.rate,length);

  p->base.dump();

  RateMonitor<DaqStats> ostats(daqStats);
  DmaStats d;
  RateMonitor<DmaStats> dstats(d);

  unsigned och0  =0;
  unsigned otot  =0;
  unsigned rxErrs=0;
  unsigned rxErrs0 = p->tpr.RxDecErrs+p->tpr.RxDspErrs;
  unsigned rxRsts=0;
  unsigned rxRsts0 = p->tpr.RxRstDone;

  p->base.start();

  while(1) {
    usleep(1000000);

    printf("--------------\n");
//     unsigned dmaStat = p->dma.rxFreeStat;
//     printf("Full/Valid/Empty : %u/%u/%u  Count :%x\n",
//            (dmaStat>>31)&1, (dmaStat>>30)&1, (dmaStat>>29)&1,
//            dmaStat&0x3ff);

    { RateMonitor<DaqStats> stats(daqStats);
      ostats.dump(stats);
      ostats = stats; }

    { unsigned v = p->tpr.RxDecErrs+p->tpr.RxDspErrs - rxErrs0;
      unsigned u = p->tpr.RxRstDone - rxRsts0;
      printf("RxErrs/Resets: %08x/%08x [%x/%x]\n", 
             v,
             u,
             v-rxErrs,
             u-rxRsts);
      rxErrs=v; rxRsts=u; }

    { unsigned uch0 = p->base.countAcquire;
      unsigned utot = p->base.countEnable;
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

  uint32_t* data = new uint32_t[1<<24];
  
  RxDesc* desc = new RxDesc(data,1<<20);
  
  ssize_t nb;

  static bool lLCLSII = targs.rate<9;

  uint64_t dpid;
  switch(targs.rate) {
  case 0: dpid = 1; break;
  case 1: dpid = 13; break;
  case 2: dpid = 91; break;
  case 3: dpid = 910; break;
  case 4: dpid = 9100; break;
  case 5: dpid = 91000; break;
  case 6: dpid = 910000; break;
  case 40:dpid = 3 ; break;
  case 41:dpid = 6 ; break;
  case 42:dpid =12 ; break;
  case 43:dpid =36 ; break;
  case 44:dpid =72 ; break;
  case 45:dpid =360; break;
  case 46:dpid =720; break;
  default: dpid = 1; break;
  }

  //  sem_post(&targs.sem);

  while(1) {
    if ((nb = read(targs.fd, desc, sizeof(*desc)))>=0) {
      { printf("READ %zd words\n",nb);
        uint32_t* p     = (uint32_t*)data;
        unsigned ilimit = lVerbose ? nb : 16;
        for(unsigned i=0; i<ilimit; i++)
          printf(" %08x",p[i]);
        printf("\n"); }
      uint32_t* p     = (uint32_t*)data;
      if ((p[1]&0xffff)==0) {
        daqStats.eventFrames()++;
        opid = p[4];
        if (lLCLSII)
          opid = (opid<<32) | p[3];
        break;
      }
    }
  }

#if 0
  pollfd pfd;
  pfd.fd      = targs.fd;
  pfd.events  = POLLIN | POLLERR;
  pfd.revents = 0;
#endif

  while (1) {
#if 0
    while (::poll(&pfd, 1, 1000)<=0)
      ;
#endif
    if ((nb = read(targs.fd, desc, sizeof(*desc)))<0) {
      //      perror("read error");
      //      break;
      //  timeout in driver
      continue;
    }

    readSize.bump(nb>>5);

    uint32_t* p     = (uint32_t*)data;
    //    uint32_t  len   = p[0];
    uint32_t  etag  = p[1];
    uint64_t  pid   = p[4]; 
    if (lLCLSII)
      pid = (pid<<32)|p[3];
    uint64_t  pid_busy = lLCLSII ? (opid + (1ULL<<20)) : (opid + 360);

    if (etag&(1ULL<<30)) {
      daqStats.dropFrames()++;
    }

    if ((etag&0xffff)==0) {
      daqStats.eventFrames()++;

      if (pid==opid) {
        daqStats.repeatFrames()++;
        printf("repeat  [%zd]: exp %016lx: ",nb,opid+dpid);
        uint32_t* p32 = (uint32_t*)data;
        for(unsigned i=0; i<8; i++)
          printf(" %08x",p32[i]);
        printf("\n"); 
      }
      else if (pid-opid != dpid) {
        daqStats.corrupt()++;
        printf("corrupt [%zd]: exp %016lx: ",nb,opid+dpid);
        uint32_t* p32 = (uint32_t*)data;
        for(unsigned i=0; i<8; i++)
          printf(" %08x",p32[i]);
        printf("\n"); 
      }

      switch(pattern) {
      case Module::Flash11:
        if (qI==QABase::Q_ABCD) {
          if (!checkFlash11_interleaved(p))
            daqStats.corrupt()++;
        }
        else {
          if (!checkFlash11(p))
            daqStats.corrupt()++;
        }
        break;
      default:
        break;
      }

      opid = pid;
      
      if (nPrint) {
        nPrint--;
        printf("EVENT  [0x%x]:",(etag&0xffff));
        unsigned ilimit = lVerbose ? nb : 16;
        for(unsigned i=0; i<ilimit; i++)
          printf(" %08x",p[i]);
        printf("\n");
      }
    
      if (targs.busyTime && opid > pid_busy) {
        usleep(targs.busyTime);
        pid_busy = lLCLSII ? (opid + (1ULL<<20)) : (opid + 360);
      }
    }
    else if ((etag&0xffff)==1) {
      printf("DMA FULL:\n");
      unsigned sz = p[0]&0xffffff;
      for(unsigned i=0; i<sz-2; i++)
        printf("%08x%c",p[2+i],(i%10)==9 ? '\n':' ');
      printf("\n");
    }
  }

  printf("read_thread done\n");

  return 0;
}

bool checkFlash11_interleaved(uint32_t* p)
{
  unsigned nb = p[0]&0xffffff;

  unsigned s=0;
  for(unsigned i=8; i<8+11*2; i++) {
    if (p[i]==0) continue;
    if (p[i]==0x07ff07ff) {
      s=i; break;
    }
    printf("Unexpected data [%08x] at word %u\n",
           p[i], i);
    return false;
  }
  if (s==0) {
    printf("No pattern found\n");
    return false;
  }

  for(unsigned i=30; i<nb; i++) {
    if ((((i-s)/2)%11)==0) {
      if (p[i] != 0x07ff07ff) {
        printf("Unexpected data %08x [%08x] at word %u:%u\n", p[i], 0x07ff07ff, i, (i-s)%22);
        return false;
      }
    }
    else if (p[i] != 0) {
      printf("Unexpected data %08x [%08x] at word %u:%u\n", p[i],0,i,(i-s)%22);
        return false;
    }
  }
  return true;
}

bool checkFlash11(uint32_t* p)
{
  unsigned nb = p[0]&0xffffff;

  const uint16_t* q = reinterpret_cast<const uint16_t*>(p+8);

  int s=-1;
  for(unsigned i=0; i<11; i++) {
    if (q[i]==0) continue;
    if (q[i]==0x07ff) {
      s=i; break;
    }
    printf("Unexpected data [%04x] at word %u\n",
           q[i], i);
    return false;
  }
  if (s==-1) {
    printf("No pattern found\n");
    return false;
  }

  nb = (nb-8)/2;

  for(unsigned j=0; j<4; j++, q+=nb) {
    for(unsigned i=s; i<nb; i++) {
      if (((i-s)%11)==0) {
        if (q[i] != 0x07ff) {
          printf("Unexpected data %04x [%04x] at word %u.%u:%u\n", q[i], 0x07ff, j, i, s);
          return false;
        }
      }
      else if (q[i] != 0) {
        printf("Unexpected data %04x [%04x] at word %u.%u:%u\n", q[i],0,j,i,s);
        return false;
      }
    }
  }
  return true;
}
