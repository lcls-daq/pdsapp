
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#include "evgr/evr/evr.hh"

#include "pds/tpr/Module.hh"

#include <string>
#include <vector>

extern int optind;

using namespace Pds::Tpr;

void usage(const char* p) {
  printf("Usage: %s -r <a/b> -R <rate> [-I] [-f <input file> | -p<channel,delay,width,polarity]>\n",p);
  printf("\t<rate>: {0=1MHz, 1=0.5MHz, 2=100kHz, 3=10kHz, 4=1kHz, 5=100Hz, 6=10Hz, 7=1Hz}\n");
}

int main(int argc, char** argv) {

  extern char* optarg;
  char evrid='a';
  std::vector<unsigned> channel;
  std::vector<unsigned> delay;
  std::vector<unsigned> width;
  std::vector<unsigned> polarity;
  TprBase::FixedRate rate = TprBase::_1K;
  bool lInternal=false;

  char* endptr;
  int c;
  bool lUsage = false;
  while ( (c=getopt( argc, argv, "Ir:f:p:R:h?")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'I':
      lInternal=true;
      break;
    case 'f':
      { FILE* f = fopen(optarg,"r");
        if (f) {
          size_t sz=4096;
          char * line = new char[sz];
          while(getline(&line,&sz,f)>=0) {
            if (line[0]!='#') {
              channel .push_back(strtoul(line    ,&endptr,0));
              delay   .push_back(strtoul(endptr+1,&endptr,0));
              width   .push_back(strtoul(endptr+1,&endptr,0));
              polarity.push_back(strtoul(endptr+1,&endptr,0));
            }
          }
          delete[] line;
        }
        else {
          perror("Failed to open file");
        }
      }
      break;
    case 'p':
      channel .push_back(strtoul(optarg  ,&endptr,0));
      delay   .push_back(strtoul(endptr+1,&endptr,0));
      width   .push_back(strtoul(endptr+1,&endptr,0));
      polarity.push_back(strtoul(endptr+1,&endptr,0));
      break;
    case 'R':
      rate = (TprBase::FixedRate)strtoul(optarg,NULL,0);
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
    
    if (lInternal) {
      p->xbar.setTpr(XBar::LoopIn);
      p->xbar.setTpr(XBar::StraightOut);
    }
    else {
      p->xbar.setTpr(XBar::StraightIn);
      p->xbar.setTpr(XBar::LoopOut);
    }
    p->xbar.dump();
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

    p->base.trigMaster=1;
    p->base.setupChannel(1, TprBase::Any, rate,
                         0, 0, 1);
    for(unsigned i=0; i<channel.size(); i++) {
      printf("Configure trigger %d for delay %d  width %d  polarity %d\n",
             channel[i],delay[i],width[i],polarity[i]);
      p->base.setupTrigger(channel[i],1,polarity[i],delay[i],width[i]);
    }
    p->base.dump();
  }

  return 0;
}

