#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvgrManager.hh"
#include "pds/evgr/EvgrPulseParams.hh"
#include "pds/evgr/EvgMasterTiming.hh"
#include "pdsapp/config/EventcodeTiming.hh"
#include "pdsapp/config/EventcodeTiming.cc"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b> -p <eventcode,delay,width,output[,polarity]> [-p ...] [-x]\n",p);
  printf("\teventcode : [40=120Hz, 41=60Hz, ..]\n");
  printf("\tdelay,width in 119MHz ticks [1=8.4ns, 2=16.8ns, ..]\n");
  printf("\toutput : connector number [0=Univ0,..]\n");
  printf("\tpolarity : 0=pos(default) | 1=neg\n");
  printf("\t [-x] : Use external sync trigger\n");
}

int main(int argc, char** argv) {

  const unsigned MAX_PULSES = 4;
  unsigned    npulses = 0;
  EvgrPulseParams pulse[MAX_PULSES];
  unsigned tooMany = 0;

  unsigned ticks;
  int delta;
  unsigned udelta;
  unsigned fiducials_per_beam = 3;
  bool external_sync=false;

  extern char* optarg;
  char* endptr;
  char* evgid=0;
  char* evrid=0;
  int c;
  while ( (c=getopt( argc, argv, "g:r:p:b:xh")) != EOF ) {
    switch(c) {
    case 'g':
      evgid = optarg;
      break;
    case 'r':
      evrid  = optarg;
      break;
    case 'p':
      if (npulses < MAX_PULSES) {
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
        printf("Pulse %u:%u,%u,%u,%u,%u\n",
            npulses,
            pulse[npulses].eventcode,
            pulse[npulses].delay,
            pulse[npulses].width,
            pulse[npulses].output,
            pulse[npulses].polarity);
        npulses++;
      } else {
        unsigned a = (unsigned)strtoul(optarg  ,&endptr,0);
        unsigned b = (unsigned)strtoul(endptr+1,&endptr,0);
        unsigned c = (unsigned)strtoul(endptr+1,&endptr,0);
        unsigned d = (unsigned)strtoul(endptr+1,&endptr,0);
        printf("Pulse %u,%u,%u,%u", a, b, c, d);
        if (*endptr==',') {
          printf(",%u", (unsigned)strtoul(endptr+1,&endptr,0));
        }
        printf(" is %u too many!\n", ++tooMany);
      }
      break;
    case 'b':
      fiducials_per_beam = strtoul(optarg,NULL,0);
      printf("Using %d fiducials per beamcode\n",fiducials_per_beam);
      break;
    case 'x':
      external_sync = true;
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    default:
      printf("Option not understood!\n");
      usage(argv[0]);
      exit(1);
    }
  }

  char defaultdev='a';
  if (!evrid) evrid = &defaultdev;
  if (!evgid) evgid = &defaultdev;

  char evgdev[16]; char evrdev[16];
  sprintf(evgdev,"/dev/eg%c3",*evgid);
  sprintf(evrdev,"/dev/er%c3",*evrid);
  printf("Using evg %s and evr %s\n",evgdev,evrdev);

  EvgrBoardInfo<Evg>& egInfo = *new EvgrBoardInfo<Evg>(evgdev);
  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  //EvgMasterTiming timing(!external_sync, 3, 120., fiducials_per_beam);
  EvgMasterTiming timing(!external_sync, 1, 360., fiducials_per_beam);
  new EvgrManager(egInfo,erInfo, timing, npulses, pulse);
  while (1) sleep(10);
  return 0;
}
