
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/service/CmdLineTools.hh"
#include "pds/config/EventcodeTiming.hh"

extern int optind;

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
  _er.InternalSequenceEnable     (0);
  _er.ExternalSequenceEnable     (0);

  _er.Reset();

  // Problem in Reset() function: It doesn't reset the set and clear masks
  // workaround: manually call the clear function to set and clear all masks
  for (unsigned ram=0;ram<2;ram++) {
    for (unsigned iopcode=0;iopcode<=EVR_MAX_EVENT_CODE;iopcode++) {
      for (unsigned jSetClear=0;jSetClear<EVR_MAX_PULSES;jSetClear++)
	_er.ClearPulseMap(ram, iopcode, jSetClear, jSetClear, jSetClear);
    }
  }    

  //  Clear all outputs for SLAC EVR (may overwrite?? in MRF EVR)
  for (unsigned k = 0; k < EVR_MAX_UNIVOUT_MAP; k++) {
    _er.SetUnivOutMap(k, EVR_MAX_PULSES-1);
  }

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
//add additional dump calls
  _er.DumpUnivOutMap(12);
  _er.DumpMapRam(ram);

  if (nsrc) {
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

int main(int argc, char** argv) {

  extern char* optarg;
  char* evrid=0;
  char* endptr;
  bool keepAlive = false;
  int c;
  unsigned ticks;
  unsigned eventcode, rate;
  bool lUsage = false;
  bool parseOK;
  int delta;
  unsigned udelta;
  while ( (c=getopt( argc, argv, "r:p:s:khT")) != EOF ) {
    switch(c) {
    case 'k':
      keepAlive = true;
      break;
    case 'r':
      evrid  = optarg;
      if (strlen(evrid) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 's':
      parseOK = false;
      endptr = index(optarg, ','); 
      if (endptr) {
        // found comma
        *endptr = '\0';
        // parse eventcode
        if (CmdLineTools::parseUInt(optarg, eventcode)) {
          // parse rate
          if (CmdLineTools::parseUInt(endptr+1, rate) && (rate > 0)) {
            parseOK = true;
            src.eventcode = eventcode;
            src.period    = 119000000/rate;
          }
        }
      }
      if (!parseOK) {
        printf("%s: option `-s' parsing error\n", argv[0]);
        lUsage = true;
      }
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
