
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#include "pds/tpr/Module.hh"

#include <string>

extern int optind;

using namespace Tpr;

struct read_args_s {
  int fd;
  int nBuffers;
  const char* filename;
};

static void* read_thread(void*);

static const unsigned NCHANNELS=12;

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b> [-R <fixedRate>] [-B <delay,width>]\n",p);
}

int main(int argc, char** argv) {

  extern char* optarg;
  char evrid='a';
  unsigned bsaDelay=0,bsaWidth=0;
  unsigned eventRate=0;
  unsigned nBuffers=1;
  const char* filename=0;

  char* endptr;
  int c;
  bool lUsage = false;
  while ( (c=getopt( argc, argv, "r:N:R:B:f:h")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'N':
      nBuffers  = strtoul(optarg,NULL,0);
      break;
    case 'R':
      eventRate = strtoul(optarg,NULL,0);
      break;
    case 'B':
      bsaDelay = strtoul(optarg,&endptr,0);
      bsaWidth = strtoul(endptr+1,&endptr,0);
      break;
    case 'f':
      filename = optarg;
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
  //  First setup the crossbar
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

    p->evr.IrqEnable(0);

    printf("Axi Version [%p]: BuildStamp: %s\n", 
	   &(p->version),
	   p->version.buildStamp().c_str());
    
    printf("[%p] [%p] [%p]\n",p, &(p->version), &(p->xbar));

    p->xbar.setTpr(XBar::StraightIn);
    p->xbar.setTpr(XBar::LoopOut);

  }

  //
  //  Setup the BSA capture
  //
  {
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
    p->tpr.rxPolarity (true);
    p->tpr.resetCounts(); 

    struct read_args_s* args = new read_args_s;
    args->fd       = fd;
    args->nBuffers = nBuffers;
    args->filename = filename;
    { 
      pthread_attr_t tattr;
      pthread_attr_init(&tattr);
      pthread_t tid;
      if (pthread_create(&tid, &tattr, &read_thread, args))
        perror("Error creating read thread");
    }

    p->base.countReset = 1;
    { timespec ts; ts.tv_sec=0; ts.tv_nsec=1000;
      nanosleep(&ts,0); }
    p->base.countReset = 0;
    p->base.setupChannel(0, TprBase::Any, TprBase::FixedRate(eventRate),
                         0, bsaDelay, bsaWidth);

    while(1) 
      sleep(1);
  }

  return 0;
}

class BsaData {
public:
  BsaData(const char* filename,
          unsigned    buffer) {
    char buff[64];
    sprintf(buff,"%s.%d",filename,buffer);
    _f = fopen(buff,"w");
    _lAcquiring = false;
  }
  ~BsaData() {}
public:
  void open(uint64_t ts) {
    _lAcquiring = true;
    fprintf(_f,"START %16lx\n",ts);
  }
  void close(uint64_t ts) {
    _lAcquiring=false;
    fflush(_f);
  }
  void event  (uint64_t pid, struct timespec& tv) {
    if (_lAcquiring)
      fprintf(_f,"%16lx\t%d.%09d\n",pid,(int)tv.tv_sec,(int)tv.tv_nsec);
  }
private:
  FILE* _f;
  bool  _lAcquiring;
};

void* read_thread(void* arg)
{
  struct read_args_s* args = reinterpret_cast<struct read_args_s*>(arg);
  int         fd       = args->fd;
  int         nBuffers = args->nBuffers;
  const char* filename = args->filename;

  BsaData* bsa[nBuffers];
  for(int i=0; i<nBuffers; i++)
    if (filename)
      bsa[i] = new BsaData(filename,i);
    else 
      bsa[i] = 0;

  uint32_t* data = new uint32_t[1024];
  
  EvrRxDesc* desc = new EvrRxDesc;
  desc->maxSize = 1024;
  desc->data    = data;
  
  unsigned eventFrames=0;
  unsigned bsaControlFrames=0;
  unsigned bsaChannelFrames[nBuffers];
  unsigned bsaChannelAcqs  [nBuffers];
  uint64_t bsaChannelMask  [nBuffers];
  unsigned dropFrames=0;

  memset(bsaChannelFrames,0,nBuffers*sizeof(unsigned));
  memset(bsaChannelAcqs  ,0,nBuffers*sizeof(unsigned));
  memset(bsaChannelMask  ,0,nBuffers*sizeof(uint64_t));

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
      if (false)
	printf("EVENT  [x%lx]: %16lx %16lx %16lx %16lx %c\n",
	       (p[0]>>48)&0xffff,p[1],p[2],p[3],p[4],m);
      break;
    case 1:

      printf("BSA_CNTL: %16lx %16lx %16lx %c\n",
             p[1],p[2],p[3],m);
      
      printf("EventFrames %8u\n", eventFrames);
      printf("DropFrames  %8u\n", dropFrames);
      printf("BsaControlF %8u\n", bsaControlFrames);
      for(int i=0; i<nBuffers; i++)
        printf("BsaChF[%u] %8u %8u %16lx\n", 
               i, bsaChannelFrames[i], bsaChannelAcqs[i], bsaChannelMask[i]);
      
      bsaControlFrames++;
      for(int i=0; i<nBuffers; i++) {
        if (!bsa[i]) continue;
        if (p[2]&(1ULL<<i))
          bsa[i]->open(p[1]);
        if (p[3]&(1ULL<<i))
          bsa[i]->close(p[1]);
      }

      break;
    case 2:
      bsaChannelFrames[(p[0]>>48)&0xf]++;
      bsaChannelMask  [(p[0]>>48)&0xf] |= p[2];
      if (p[2]!=0)
	bsaChannelAcqs[(p[0]>>48)&0xf]++;

      if (false)
	printf("BSA_CHAN[%lu]: %16lx %16lx %16lx %c\n",
	       (p[0]>>48)&0xff,p[1],p[2],p[3],m);

      struct timespec ts;
      clock_gettime(CLOCK_REALTIME,&ts);
      for(int i=0; i<nBuffers; i++)
        if (p[2]&(1ULL<<i) && bsa[i])
          bsa[i]->event(p[1],ts);

      break;
    default:
      break;
    }
  }

  return 0;
}
