
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

#include <string>

extern int optind;

using namespace Pds;

class TprCore {
public:
  void dump() const {
    struct timespec ts; ts.tv_sec=1; ts.tv_nsec=0;
    unsigned v0,v1;
    for(unsigned i=0; i<10; i++) {
      v0 = RxRecClks;
      nanosleep(&ts,0);
      v1 = RxRecClks;
      unsigned d = v1-v0;
      double r = double(d)*16.e-6;
      double dr = r - 1300/7.;
      printf("RxRecClkRate: %7.4f MHz [%7.4f]\n",r,dr);
    }
  }
public:
  volatile uint32_t SOFcounts;
  volatile uint32_t EOFcounts;
  volatile uint32_t Msgcounts;
  volatile uint32_t CRCerrors;
  volatile uint32_t RxRecClks;
  volatile uint32_t RxRstDone;
  volatile uint32_t RxDecErrs;
  volatile uint32_t RxDspErrs;
  volatile uint32_t CSR;
};

// Memory map of TPR registers (EvrCardG2 BAR 1)
class TprReg {
public:
  volatile uint32_t reserved [(0x40000)/4];
  TprCore  tpr;
};

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b>\n",p);
}

int main(int argc, char** argv) {

  extern char* optarg;
  char evrid='a';

  char* endptr;
  int c;
  bool lUsage = false;
  bool parseOK;
  while ( (c=getopt( argc, argv, "r:h")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
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
    p->tpr.dump();
  }

  return 0;
}

