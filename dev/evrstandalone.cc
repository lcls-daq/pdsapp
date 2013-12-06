
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pdsapp/config/EventcodeTiming.hh"
#include "pdsapp/config/EventcodeTiming.cc"

using namespace Pds;

static EvgrBoardInfo<Evr> *erInfoGlobal;

class PulseParams {
public:
  unsigned eventcode;
  unsigned polarity;
  unsigned delay;
  unsigned width;
  unsigned output;
};

const unsigned MAX_PULSES = 10;
unsigned    npulses = 0;
PulseParams pulse[MAX_PULSES];

class SrcParams {
public:
  unsigned eventcode;
  unsigned period;
};

unsigned nsrc = 0;
SrcParams src;

//
//  AMO : {41,0,11900,8}  // pnCCD clear
//        {41,0,11900,9}  // pnCCD clear
//        {42,0,   11,7}  // YAG laser
//

class EvrStandAloneManager {
public:
  EvrStandAloneManager(EvgrBoardInfo<Evr>& erInfo);
  void configure();
  void start();
private:
  Evr& _er;
};

void EvrStandAloneManager::start() {
  unsigned ram=0;
  _er.MapRamEnable(ram,1);
};

void EvrStandAloneManager::configure() {
  printf("Configuring evr\n");
  _er.Reset();

  // setup map ram
  int ram=0;
  _er.MapRamEnable(ram,0);

  for(unsigned i=0; i<npulses; i++) {
    printf("Configuring pulse %d  eventcode %d  delay %d  width %d  output %d\n",
           i, pulse[i].eventcode, pulse[i].delay, pulse[i].width, pulse[i].output);
    _er.SetPulseMap(ram, pulse[i].eventcode, i, -1, -1);
    _er.SetPulseProperties(i, pulse[i].polarity, 0, 0, 1, 1);
    _er.SetPulseParams(i,1,pulse[i].delay,pulse[i].width);
    _er.SetUnivOutMap( pulse[i].output, i);
  }

  _er.DumpPulses(npulses);

  if (nsrc) {
    _er.InternalSequenceEnable(0);
    _er.ExternalSequenceEnable(0);
    if (src.period) {
      _er.InternalSequenceSetCode    (src.eventcode);
      _er.InternalSequenceSetPrescale(src.period-1);
      _er.InternalSequenceEnable     (1);
    }
    else {
      _er.ExternalSequenceSetCode (src.eventcode);
      _er.ExternalSequenceEnable  (1);
    }
  }

  _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
  _er.EnableFIFO(1);
  _er.Enable(1);
}

extern "C" {
  void evrsa_sig_handler(int parm)
  {
    Evr& er = erInfoGlobal->board();
    int flags = er.GetIrqFlags();
    if (flags & EVR_IRQFLAG_EVENT)
      {
        FIFOEvent fe;
        er.GetFIFOEvent(&fe);
        printf("Received event %x %x\n",fe.TimestampLow,fe.TimestampHigh);
        er.ClearIrqFlags(EVR_IRQFLAG_EVENT);
      }
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

EvrStandAloneManager::EvrStandAloneManager(EvgrBoardInfo<Evr> &erInfo) :
  _er(erInfo.board()) {

  configure();
  start();

  _er.IrqAssignHandler(erInfo.filedes(), &evrsa_sig_handler);
  erInfoGlobal = &erInfo;
}

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b> -s <eventcode,rate> -p <eventcode,delay,width,output[,polarity]> [-p ...] [-k]\n",p);
  printf("\teventcode : [40=120Hz, 41=60Hz, ..]\n");
  printf("\tdelay,width in 119MHz ticks [1=8.4ns, 2=16.8ns, ..]\n");
  printf("\toutput : connector number [0=Univ0,..]\n");
  printf("\tpolarity : 0=pos(default) | 1=neg\n");
  printf("\t-k makes the program immortal\n");
}

class Pds_ConfigDb::EventcodeTiming;

int main(int argc, char** argv) {

  extern char* optarg;
  char* evrid=0;
  char* endptr;
  bool keepAlive = false;
  int c;
  unsigned ticks;
  int delta;
  unsigned udelta;
  while ( (c=getopt( argc, argv, "r:p:s:khT")) != EOF ) {
    switch(c) {
    case 'k':
      keepAlive = true;
      break;
    case 'r':
      evrid  = optarg;
      break;
    case 's':
      src.eventcode = strtoul(optarg,&endptr,0);
      src.period    = 119000000/strtoul(endptr+1,&endptr,0);
      nsrc++;
      break;
    case 'p':
      pulse[npulses].eventcode = strtoul(optarg  ,&endptr,0);
      ticks = Pds_ConfigDb::EventcodeTiming::timeslot(pulse[npulses].eventcode);
      delta = ticks - Pds_ConfigDb::EventcodeTiming::timeslot(140);
      udelta = abs(delta);
      pulse[npulses].delay     = strtoul(endptr+1,&endptr,0);
      if (pulse[npulses].delay>udelta) pulse[npulses].delay -= delta;
      pulse[npulses].width     = strtoul(endptr+1,&endptr,0);
      pulse[npulses].output    = strtoul(endptr+1,&endptr,0);
      if (*endptr==',') {
        pulse[npulses].polarity = strtoul(endptr+1,&endptr,0);
      } else {
        pulse[npulses].polarity = 0;
      }
      if (npulses>=4 && pulse[npulses].width>0xffff) 
        printf("Pulse %d width exceeds EVR limit of 16-bits.  Truncating!\n",npulses);

      npulses++;
      break;
    case 'T':
      printf("Event code\tticks\tmicroseconds\n");
      for (unsigned code=0; code<255; code++) {
        unsigned ticks = Pds_ConfigDb::EventcodeTiming::timeslot(code);
        long double ns = (long double)8.4 / 1000.0;
        if (ticks) {
          printf("%8u\t%d\t%7.3Lf\n", code, ticks, ns*ticks);
        }
      }
      return 0;
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    }
  }

  char defaultdev='a';
  if (!evrid) evrid = &defaultdev;

  char evrdev[16];
  sprintf(evrdev,"/dev/er%c3",*evrid);
  printf("Using evr %s\n",evrdev);

  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  new EvrStandAloneManager(erInfo);
  while (keepAlive) {
    sleep(10);
  }
  return 0;
}
