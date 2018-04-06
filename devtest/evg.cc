#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvgManager.hh"
#include "pds/evgr/EvgMasterTiming.hh"
#include "pds/config/EventcodeTiming.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -g <evg a/b> [-b N] [-x]\n",p);
  printf("\t [-x] : Use external sync trigger\n");
  printf("\t [-b] : Set fiducials per beamcode\n");
}

int main(int argc, char** argv) {
  unsigned fiducials_per_beam = 3;
  bool external_sync=false;

  extern char* optarg;
  char* evgid=0;
  int c;
  while ( (c=getopt( argc, argv, "b:g:xh")) != EOF ) {
    switch(c) {
    case 'g':
      evgid = optarg;
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
  if (!evgid) evgid = &defaultdev;

  char evgdev[16];
  sprintf(evgdev,"/dev/eg%c3",*evgid);
  printf("Using evg %s\n",evgdev);

  EvgrBoardInfo<Evg>& egInfo = *new EvgrBoardInfo<Evg>(evgdev);
  //EvgMasterTiming timing(!external_sync, 3, 120., fiducials_per_beam);
  EvgMasterTiming timing(!external_sync, 1, 360., fiducials_per_beam);
  new EvgManager(egInfo, timing);
  while (1) sleep(10);
  return 0;
}
