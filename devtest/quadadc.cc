
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

#include "pds/quadadc/Module.hh"

#include <string>

extern int optind;

using namespace Pds::QuadAdc;

void usage(const char* p) {
  printf("Usage: %s [options]\n",p);
  printf("Options: -d <dev id>\n");
  printf("\t-C <initialize clock synthesizer>\n");
  printf("\t-R <reset timing frame counters>\n");
  printf("\t-X <reset gtx timing receiver>\n");
  printf("\t-P <reverse gtx rx polarity>\n");
  printf("\t-A <dump gtx alignment>\n");
  printf("\t-0 <dump raw timing receive buffer>\n");
  printf("\t-1 <dump timing message buffer>\n");
  //  printf("Options: -a <IP addr (dotted notation)> : Use network <IP>\n");
}

int main(int argc, char** argv) {

  extern char* optarg;

  char qadc='a';
  int c;
  bool lUsage = false;
  bool lSetupClkSynth = false;
  bool lReset = false;
  bool lResetRx = false;
  bool lPolarity = false;
  bool lRing0 = false;
  bool lRing1 = false;
  bool lDumpAlign = false;
  int  regTest=-1, regValu=-1;

  while ( (c=getopt( argc, argv, "CRXPA01d:hT:V:")) != EOF ) {
    switch(c) {
    case 'A':
      lDumpAlign = true;
      break;
    case 'C':
      lSetupClkSynth = true;
      break;
    case 'P':
      lPolarity = true;
      break;
    case 'R':
      lReset = true;
      break;
    case 'X':
      lResetRx = true;
      break;
    case '0':
      lRing0 = true;
      break;
    case '1':
      lRing1 = true;
      break;
    case 'd':
      qadc = optarg[0];
      break;
    case 'T':
      regTest = strtoul(optarg,NULL,0);
      break;
    case 'V':
      regValu = strtoul(optarg,NULL,0);
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

  char devname[16];
  sprintf(devname,"/dev/qadc%c",qadc);
  int fd = open(devname, O_RDWR);
  if (fd<0) {
    perror("Open device failed");
    return -1;
  }

  void* ptr = mmap(0, sizeof(Pds::QuadAdc::Module), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Failed to map");
    return -2;
  }
  
  Module* p = reinterpret_cast<Module*>(ptr);

  printf("Axi Version [%p]: BuildStamp[%p]: %s\n", 
         &(p->version), &(p->version.BuildStamp[0]), p->version.buildStamp().c_str());

  p->dma_core.init(0);
  p->dma_core.dump();

  p->phy_core.dump();

  p->i2c_sw_control = 8;
  printf("I2C Switch: %x\n", p->i2c_sw_control);

  //  p->clksynth.dump();

  if (regTest>=0) {
    if (regTest>255)
      p->clksynth._reg[255]=0x1;
    else
      p->clksynth._reg[255]=0x0;
    printf("Reading register 0x%x\n",regTest&0xff);
    unsigned qq = -1;
    while(1) {
      unsigned q = p->clksynth._reg[regTest&0xff];
      if (qq != q) {
        printf(":%02x\n",q);
        qq=q;
      }
      if (regValu>=0)
        p->clksynth._reg[regTest&0xff] = regValu&0xff;
    }
  }

  if (lSetupClkSynth) {
    p->clksynth.setup();
    p->clksynth.dump();
  }

  if (lPolarity) {
    p->tpr.rxPolarity(!p->tpr.rxPolarity());
  }

  if (lResetRx) {
    p->tpr.resetRxPll();
    usleep(10000);
  }

  if (lReset) {
    p->tpr.resetCounts();
    usleep(10000);
  }

  printf("TPR [%p]\n", &(p->tpr));
  p->tpr.dump();

  for(unsigned i=0; i<5; i++) {
    timespec tvb;
    clock_gettime(CLOCK_REALTIME,&tvb);
    unsigned vvb = p->tpr.TxRefClks;

    usleep(10000);

    timespec tve;
    clock_gettime(CLOCK_REALTIME,&tve);
    unsigned vve = p->tpr.TxRefClks;
    
    double dt = double(tve.tv_sec-tvb.tv_sec)+1.e-9*(double(tve.tv_nsec)-double(tvb.tv_nsec));
    printf("TxRefClk rate = %f MHz\n", 16.e-6*double(vve-vvb)/dt);
  }

  for(unsigned i=0; i<5; i++) {
    timespec tvb;
    clock_gettime(CLOCK_REALTIME,&tvb);
    unsigned vvb = p->tpr.RxRecClks;

    usleep(10000);

    timespec tve;
    clock_gettime(CLOCK_REALTIME,&tve);
    unsigned vve = p->tpr.RxRecClks;
    
    double dt = double(tve.tv_sec-tvb.tv_sec)+1.e-9*(double(tve.tv_nsec)-double(tvb.tv_nsec));
    printf("RxRecClk rate = %f MHz\n", 16.e-6*double(vve-vvb)/dt);
  }

  if (lRing0) {
    p->ring0.enable(false);
    p->ring0.clear ();
    p->ring0.enable(true);
    usleep(1000);
    p->ring0.enable(false);
    p->ring0.dump();
  }

  if (lRing1) {
    p->ring1.enable(false);
    p->ring1.clear();
    p->ring1.enable(true);
    usleep(1000);
    p->ring1.enable(false);
    p->ring1.dump();
  }

  if (lDumpAlign) {
    for(unsigned i=0; i<20; i++)
      printf(" %04x",(p->gthAlign[i/10] >> (16*(i&1)))&0xffff);
    printf("\nTarget: %u\n",p->gthAlignTarget);
  }

  return 0;
}
