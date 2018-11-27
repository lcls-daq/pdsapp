#include "pdsapp/devtest/PgpDevice.hh"
#include "pdsapp/devtest/AxiMicronN25Q.hh"
#include "pdsapp/devtest/AxiCypressS25.hh"
#include "pds/pgp/SrpV3.hh"
#include "pds/pgp/Destination.hh"
#include <PgpDriver.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

using namespace PdsApp;

void printUsage(char* name) {
  printf( "Usage: %s [-h]  -P <cardNumb,vc,lane> -f <mcs file> -a <addr> -w\n",
          name );
}

int main( int argc, char** argv ) {
  unsigned            pgpcard             = 0;
  unsigned            addr                = 0;
  unsigned            vc                  = 0;
  unsigned            lane                = 0;
  const char*         mcsfile             = 0;
  bool                lwrite              = false;

  char*               endptr;
  extern char*        optarg;
  int c;
  while( ( c = getopt( argc, argv, "hP:f:a:w") ) != EOF ) {
    switch(c) {
    case 'P':
      pgpcard = strtoul(optarg  ,&endptr,0);
      vc      = strtoul(endptr+1,&endptr,0);
      lane    = strtoul(endptr+1,&endptr,0);
      break;
    case 'f':
      mcsfile = optarg;
      break;
    case 'a':
      addr = strtoul(optarg, &endptr, 0);
      break;
    case 'w':
      lwrite = true;
      break;
    default:
      printUsage(argv[0]);
      return 0;
    }
  }

  char err[128];
  char devName[64];
  sprintf(devName, "/dev/pgpcard_%u",pgpcard);

  int fd = open( devName,  O_RDWR );
  printf("%s using %s\n", argv[0], devName);
  if (fd < 0) {
    sprintf(err, "%s opening %s failed", argv[0], devName);
    perror(err);
    // What else to do if the open fails?
    return 1;
  }

  Pds::Pgp::SrpV3::Protocol* srp = new Pds::Pgp::SrpV3::Protocol(fd,lane);
  unsigned                  dst((lane<<2)|vc);
  { unsigned char maskBytes[32];
    dmaInitMaskBytes(maskBytes);
    dmaAddMaskBytes(maskBytes,dst);
    dmaSetMaskBytes(fd, maskBytes); }

  PgpDevice device(*srp, vc, addr);

  //  AxiMicronN25Q prom(mcsfile, device);
  AxiCypressS25 prom(mcsfile, device);

  if (lwrite) prom.load();

  prom.verify();

  return 0;
}
