#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvgrManager.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

int main(int argc, char** argv) {

  extern char* optarg;
  char* evgid=0;
  char* evrid=0;
  int c;
  while ( (c=getopt( argc, argv, "g:r:")) != EOF ) {
    switch(c) {
    case 'g':
      evgid = optarg;
      break;
    case 'r':
      evrid  = optarg;
      break;
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
  new EvgrManager(egInfo,erInfo);
  while (1) sleep(10);
  return 0;
}
