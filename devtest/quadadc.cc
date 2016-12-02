
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include <new>

#include "pds/quadadc/Globals.hh"
#include "pds/quadadc/Module.hh"

#include <string>

extern int optind;

using namespace Pds::QuadAdc;
using Pds::Tpr::RxDesc;

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
  char* endptr;

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
  bool lTrain = false;
  bool lSetPhase = false;
  bool lTestSync = false;
  unsigned syncDelay[4] = {0,0,0,0};

  const char* fWrite=0;
  unsigned delay_int=0, delay_frac=0;
  unsigned alignTarget = 16;
  int      alignRstLen = -1;

  int  regTest=-1, regValu=-1;

  while ( (c=getopt( argc, argv, "CRS:XPA:01D:d:hTW:")) != EOF ) {
    switch(c) {
    case 'A':
      lDumpAlign = true;
      alignTarget = strtoul(optarg,&endptr,0);
      if (endptr[0])
        alignRstLen = strtoul(endptr+1,NULL,0);
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
    case 'S':
      lTestSync = true;
      syncDelay[0] = strtoul(optarg,&endptr,0);
      for(unsigned i=1; i<4; i++)
        syncDelay[i] = strtoul(endptr+1,&endptr,0);
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
      lTrain = true;
      break;
    case 'D':
      lSetPhase = true;
      delay_int  = strtoul(optarg,&endptr,0);
      if (endptr[0]) {
        delay_frac = strtoul(endptr+1,&endptr,0);
      }
      break;
    case 'W':
      fWrite = optarg;
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

  p->i2c_sw_control.select(I2cSwitch::LocalBus);
  p->i2c_sw_control.dump();
  
  printf("Local CPLD revision: 0x%x\n", p->local_cpld.revision());
  printf("Local CPLD GAaddr  : 0x%x\n", p->local_cpld.GAaddr  ());
  p->local_cpld.GAaddr(0);

  printf("vtmon1 mfg:dev %x:%x\n", p->vtmon1.manufacturerId(), p->vtmon1.deviceId());
  printf("vtmon2 mfg:dev %x:%x\n", p->vtmon2.manufacturerId(), p->vtmon2.deviceId());
  printf("vtmon3 mfg:dev %x:%x\n", p->vtmon3.manufacturerId(), p->vtmon3.deviceId());

  printf("FMC A [%p]: %s present power %s\n",
         &p->fmc_core,
         p->fmc_core.present() ? "":"not",
         p->fmc_core.powerGood() ? "up":"down");
  printf("AdcCore[%p]\n",&p->adc_core);
  printf("FmcCore[%p]\n",&p->fmc_core);

  p->i2c_sw_control.select(I2cSwitch::PrimaryFmc); 
  p->i2c_sw_control.dump();

  printf("vtmona mfg:dev %x:%x\n", p->vtmona.manufacturerId(), p->vtmona.deviceId());

  if (lTrain) {
    p->fmc_init();
    p->train_io();
  }

  p->base.dump();
#if 0
  if (lSetPhase) {
    printf("QABase adcSync %x\n", p->base.adcSync);
    if (lLCLS)
      p->setClockLCLS  (delay_int,delay_frac);
    else
      p->setClockLCLSII(delay_int,delay_frac);
    printf("QABase adcSync %x\n", p->base.adcSync);
  }
  else {
    printf("QABase clock %s\n", p->base.clockLocked() ? "locked" : "not locked");
    printf("QABase adcSync %x\n", p->base.adcSync);
  }
#endif

  for(unsigned i=0; i<8; i++) {
    p->fmc_core.selectClock(i);
    usleep(100000);
    printf("Clock [%i]: rate %f MHz\n", i, p->fmc_core.clockRate()*1.e-6);
  }

  p->dma_core.init(1<<20);
  p->dma_core.dump();

  p->phy_core.dump();

  p->i2c_sw_control.select(I2cSwitch::LocalBus);  // ClkSynth is on local bus
  p->i2c_sw_control.dump();

  if (lSetupClkSynth) {
    p->clksynth.setup();
    p->clksynth.dump();
  }

  if (lPolarity) {
    p->tpr.rxPolarity(!p->tpr.rxPolarity());
  }

  if (lResetRx) {
#ifdef LCLSII
    p->tpr.setLCLSII();
#else
    p->tpr.setLCLS();
#endif
    p->tpr.resetRxPll();
    usleep(10000);
    p->tpr.resetRx();
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
    usleep(100000);
    p->ring1.enable(false);
    p->ring1.dump();
  }

  if (lTestSync) {
    uint32_t* data = new uint32_t[1<<20];
    RxDesc* desc = new RxDesc(data,1<<20);
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    unsigned n=0;
    while(poll(&pfd,1,0)>0) { 
      n++;
      read(fd, desc, sizeof(*desc));
    }
    printf("flushed %u\n",n);

    p->dma_core.init(32+32*48);
    p->adc_sync.set_delay(syncDelay);
    p->adc_sync.start_training();
    p->base.init();
    p->base.setupLCLS(9,32);
    p->base.start();

    struct timespec tv;
    clock_gettime(CLOCK_REALTIME,&tv);

    n=0;
    while(poll(&pfd,1,1000)>0) { 
      n++;
      read(fd, desc, sizeof(*desc));
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME,&ts);
      if (ts.tv_sec > tv.tv_sec+1)
        break;
    }
    printf("Read %x events\n",n);
    p->base.stop();
    p->adc_sync.stop_training();
    p->adc_sync.dump_status();
    while(poll(&pfd,1,0)>0) { 
      read(fd, desc, sizeof(*desc));
    }
    delete desc;
    delete[] data;
  }

  if (lDumpAlign) {
    p->dumpRxAlign();
    if (alignTarget < 128)
      p->setRxAlignTarget(alignTarget);
    if (alignRstLen > 0)
      p->setRxResetLength(alignRstLen);
  }

  if (fWrite) {
    FILE* f = fopen(fWrite,"r");
    if (f)
      p->flash.write(f);
    else 
      perror("Failed opening prom file\n");
  }

  return 0;
}
