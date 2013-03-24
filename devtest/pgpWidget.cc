#include "pds/pgp/Pgp.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/DataImportFrame.hh"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <new>

void sigHandler( int signal ) {
  psignal( signal, "Signal received by pgpWidget");
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}



using namespace Pds;

Pgp::Destination* dest;
Pgp::Pgp* pgp;
Pds::Pgp::RegisterSlaveImportFrame* rsif;

void printUsage(char* name) {
  printf( "Usage: %s [-h]  -P <pgpcardNumb> [-w dest,addr,data][-r dest,addr][-R][-D <debug>] [-f <runTimeConfigName>][-p pf]\n"
      "    -h      Show usage\n"
      "    -P      Set pgpcard index number  (REQUIRED)\n"
      "                The format of the index number is a one byte number with the bottom nybble being\n"
      "                the index of the card and the top nybble being a port mask where one bit is for\n"
      "                each port, but a value of zero maps to 15 for compatibility with unmodified\n"
      "                applications that use the whole card\n"
      "    -w      Write register to destination, reply will be send to standard output\n"
      "                The format of the paraemeters are: 'dest addr data'\n"
      "                where addr and Data are 32 bit unsigned integers, but the Dest is a\n"
      "                four bit field where the bottom two bits are VC and The top two are Lane\n"
      "    -r      Read register from destination, resulting data is written to the standard output\n"
      "                The format of the paraemeters are: 'dest addr'\n"
      "                where addr is a 32 bit unsigned integer, but the Dest is a four bit field,\n"
      "                where the bottom two bits are VC and The top two are Lane\n"
      "    -R      Loop reading data until interrupted, resulting data will be written to the standard output.\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          print out progress\n"
       "    -f      set run time config file name\n"
      "                The format of the file consists of lines: 'Dest Addr Data'\n"
      "                where Addr and Data are 32 bit unsigned integers, but the Dest is a\n"
      "                four bit field where the bottom two bits are VC and The top two are Lane\n"
      "    -p      set print flag to value given\n",
      name
  );
}

enum Commands{none, writeCommand,readCommand,readAsyncCommand,numberOfCommands};

int main( int argc, char** argv )
{
  unsigned            pgpcard             = 0;
  unsigned            d                   = 0;
  uint32_t            data                = 0x1234;
  unsigned            command             = none;
  unsigned            addr                = 0;
  unsigned            printFlag           = 0;
  bool                cardGiven = false;
  unsigned            debug               = 0;
  ::signal( SIGINT, sigHandler );
  char                runTimeConfigname[256] = {""};

  char* endptr;
  extern char* optarg;
  int c;
  while( ( c = getopt( argc, argv, "hP:w:D:P:r:Rf:p:" ) ) != EOF ) {
     switch(c) {
      case 'P':
        pgpcard = strtoul(optarg, NULL, 0);
        cardGiven = true;
        break;
      case 'D':
        debug = strtoul(optarg, NULL, 0);
        break;
      case 'f':
        strcpy(runTimeConfigname, optarg);
        break;
      case 'w':
        command = writeCommand;
        d = strtoul(optarg  ,&endptr,0);
        addr = strtoul(endptr+1,&endptr,0);
        data = strtoul(endptr+1,&endptr,0);
        break;
      case 'r':
        command = readCommand;
        d = strtoul(optarg  ,&endptr,0);
        addr = strtoul(endptr+1,&endptr,0);
        if (debug & 1) printf("\t read %u,0x%x\n", d, addr);
        break;
      case 'R':
        command = readAsyncCommand;
        break;
      case 'p':
        printFlag = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        printUsage(argv[0]);
        return 0;
        break;
      default:
        printf("Error: Option could not be parsed, or is not supported yet!\n");
        printUsage(argv[0]);
        return 0;
        break;
    }
  }

  if (!cardGiven) {
    printf("PGP card must be specified !!\n");
    printUsage(argv[0]);
    return 1;
  }
  unsigned ports = (pgpcard >> 4) & 0xf;
  char devName[128];
  char err[128];
  if (ports == 0) {
    ports = 15;
    sprintf(devName, "/dev/pgpcard%u", pgpcard);
  } else {
    sprintf(devName, "/dev/pgpcard_%u_%u", pgpcard & 0xf, ports);
  }

  int fd = open( devName,  O_RDWR );
  if (debug & 1) printf("pgpwidget using %s\n", devName);
  if (fd < 0) {
    sprintf(err, "pgpWidget opening %s failed", devName);
    perror(err);
    // What else to do if the open fails?
    return 1;
  }

  unsigned offset = 0;
  while ((((ports>>offset) & 1) == 0) && (offset < 5)) {
    offset += 1;
  }

  Pds::Pgp::Pgp::portOffset(offset);

  pgp = new Pds::Pgp::Pgp::Pgp(fd, debug != 0);
  dest = new Pds::Pgp::Destination::Destination(d);

  if (debug & 1) printf("pgpWidget destination %s\n", dest->name());
  bool keepGoing = true;
  unsigned ret = 0;

  switch (command) {
    case none:
      printf("pgpWidget - no command given, exiting\n");
      return 0;
      break;
    case writeCommand:
      pgp->writeRegister(dest, addr, data, printFlag, Pds::Pgp::PgpRSBits::Waiting);
      rsif = pgp->read();
      if (rsif) {
        if (debug & 1) rsif->print();
        printf("pgpWidget write returned 0x%x\n", rsif->_data);
      }
      break;
    case readCommand:
      ret = pgp->readRegister(dest, addr,0x2dbeef, &data, 1, printFlag);
      if (!ret) printf("pgpWidget read returned 0x%x\n", data);
      break;
    case readAsyncCommand:
      enum {BufferWords = 1<<24};
//      Pds::Pgp::DataImportFrame* inFrame;
      PgpCardRx       pgpCardRx;
      pgpCardRx.model   = sizeof(&pgpCardRx);
      pgpCardRx.maxSize = BufferWords;
      pgpCardRx.data    = (uint32_t*)malloc(BufferWords);
      int readRet;
      while (keepGoing) {
        if ((readRet = ::read(fd, &pgpCardRx, sizeof(PgpCardRx))) >= 0) {
          unsigned loopCount = readRet < 64 ? readRet : 16;
          for (unsigned i=0; i<loopCount; i++) printf("0x%0x ", pgpCardRx.data[i]);
          printf("\n");
        } else {
          perror("pgpWidget Async reading error");
          keepGoing = false;
        }
      }
      break;
    default:
      printf("pgpWidget - unknown command, exiting\n");
      break;
  }

  return 0;
}
