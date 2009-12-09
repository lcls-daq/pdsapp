
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"

using namespace Pds;

static EvgrBoardInfo<Evr> *erInfoGlobal;

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
  int ram=0; int enable=1;
  _er.MapRamEnable(ram,0);
  int opcode=45;
  _er.SetFIFOEvent(ram, opcode, enable);
  int trig=0; int set=-1; int clear=-1;
  _er.SetPulseMap(ram, opcode, trig, set, clear);

  int pulse = 0; int presc = 1;
  int polarity=0;  int map_reset_ena=0; int map_set_ena=0; int map_trigger_ena=1;
  _er.SetPulseProperties(pulse, polarity, map_reset_ena, map_set_ena, map_trigger_ena,
                           enable);
  const double tickspersec = 119.0e6;

  int delay=(int)(0.0*tickspersec);
  int width=(int)(100.e-6*tickspersec);
  _er.SetPulseParams(pulse,presc,delay,width);

  _er.SetUnivOutMap( 8, pulse);
  _er.SetUnivOutMap( 9, pulse);

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

int main(int argc, char** argv) {

  extern char* optarg;
  char* evrid=0;
  int c;
  while ( (c=getopt( argc, argv, "g:r:")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg;
      break;
    }
  }

  char defaultdev='a';
  if (!evrid) evrid = &defaultdev;

  char evrdev[16];
  sprintf(evrdev,"/dev/er%c3",*evrid);
  printf("Using evr %s\n",evrdev);

  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  new EvrStandAloneManager(erInfo);
  while (1) sleep(10);
  return 0;
}
