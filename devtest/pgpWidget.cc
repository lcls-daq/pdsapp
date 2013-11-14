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

FILE*               writeFile           = 0;
bool                writing             = false;

void sigHandler( int signal ) {
  psignal( signal, "Signal received by pgpWidget");
  if (writing) fclose(writeFile);
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}



using namespace Pds;

Pgp::Destination* dest;
Pgp::Pgp* pgp;
Pds::Pgp::RegisterSlaveImportFrame* rsif;

void printUsage(char* name) {
  printf( "Usage: %s [-h]  -P <pgpcardNumb> [-w dest,addr,data][-W dest,addr,data,count,delay][-r dest,addr][-d dest,addr,count][-t dest,addr,count][-R][-o maxPrint][-s filename][-D <debug>] [-f <runTimeConfigName>][-p pf]\n"
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
      "    -W      Loop writing register to destination count times, unless count = 0 which means forever\n"
      "                 delay is time between packets sent in microseconds\n"
      "                 other parameters are the same as the write command above\n"
      "    -r      Read register from destination, resulting data, is written to the standard output\n"
      "                The format of the paraemeters are: 'dest addr'\n"
      "                where addr is a 32 bit unsigned integer, but the Dest is a four bit field,\n"
      "                where the bottom two bits are VC and The top two are Lane\n"
      "    -t      Test registers, loop from addr counting up count times\n"
      "    -d      Dump count registers starting at addr\n"
      "    -R      Loop reading data until interrupted, resulting data will be written to the standard output.\n"
      "    -o      Print out up to maxPrint words when reading data\n"
      "    -s      Save to file when reading data\n"
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

enum Commands{none, writeCommand,readCommand,readAsyncCommand,dumpCommand,testCommand,loopWriteCommand,numberOfCommands};

int main( int argc, char** argv )
{
  unsigned            pgpcard             = 0;
  unsigned            d                   = 0;
  uint32_t            data                = 0xdead;
  unsigned            command             = none;
  unsigned            addr                = 0;
  unsigned            count               = 0;
  unsigned            delay               = 1000;
  unsigned            printFlag           = 0;
  unsigned            maxPrint            = 0;
  bool                cardGiven           = false;
  unsigned            debug               = 0;
  unsigned            idx                 = 0;
  ::signal( SIGINT, sigHandler );
  char                runTimeConfigname[256] = {""};
  char                writeFileName[256] = {""};
  char*               endptr;
  extern char*        optarg;
  int c;
  while( ( c = getopt( argc, argv, "hP:w:W:D:P:r:d:Ro:s:f:p:t:" ) ) != EOF ) {
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
      case 'W':
        command = loopWriteCommand;
        d = strtoul(optarg  ,&endptr,0);
        addr = strtoul(endptr+1,&endptr,0);
        data = strtoul(endptr+1,&endptr,0);
        count = strtoul(endptr+1,&endptr,0);
        delay = strtoul(endptr+1,&endptr,0);
//        printf("loopWriteCommand %u,%u,0x%x,%u\n", d, addr, data, count);
        break;
     case 'r':
        command = readCommand;
        d = strtoul(optarg  ,&endptr,0);
        addr = strtoul(endptr+1,&endptr,0);
        if (debug & 1) printf("\t read %u,0x%x\n", d, addr);
        break;
      case 'd':
        command = dumpCommand;
        d = strtoul(optarg  ,&endptr,0);
        addr = strtoul(endptr+1,&endptr,0);
        count = strtoul(endptr+1,&endptr,0);
        break;
      case 't':
        command = testCommand;
        d = strtoul(optarg  ,&endptr,0);
        addr = strtoul(endptr+1,&endptr,0);
        count = strtoul(endptr+1,&endptr,0);
        break;
      case 'R':
        command = readAsyncCommand;
        break;
      case 'o':
        maxPrint = strtoul(optarg, NULL, 0);
        break;
      case 's':
        strcpy(writeFileName, optarg);
        writing = true;
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

  if (writing) {
//    char path[512];
//    char* home = getenv("HOME");
//    sprintf(path,"%s/%s",home, writeFileName);
    printf("Opening %s for writing to\n", writeFileName);
    writeFile = fopen(writeFileName, "w+");
    if (!writeFile) {
      char s[200];
      sprintf(s, "Could not open %s ", writeFileName);
      perror(s);
      return 1;
    }
  }

  unsigned offset = 0;
  while ((((ports>>offset) & 1) == 0) && (offset < 5)) {
    offset += 1;
  }

  Pds::Pgp::Pgp::portOffset(offset);

  pgp = new Pds::Pgp::Pgp::Pgp(fd, debug != 0);
  dest = new Pds::Pgp::Destination::Destination(d);

  if (strlen(runTimeConfigname)) {
    FILE* f;
    Pgp::Destination _d;
    unsigned maxCount = 1024;
    char path[240];
    char* home = getenv("HOME");
    sprintf(path,"%s/%s",home, runTimeConfigname);
    printf("Configuring from %s", path);
    f = fopen (path, "r");
    if (!f) {
      char s[200];
      sprintf(s, "Could not open %s ", path);
      perror(s);
    } else {
      unsigned myi = 0;
      unsigned dest, addr, data;
      while (fscanf(f, "%x %x %x", &dest, &addr, &data) && !feof(f) && myi++ < maxCount) {
        _d.dest(dest);
        printf("\nConfig from file, dest %s, addr 0x%x, data 0x%x ", _d.name(), addr, data);
        if(pgp->writeRegister(&_d, addr, data)) {
          printf("\npgpwidget writing config failed on dest %u address 0x%x\n", dest, addr);
        }
      }
      if (!feof(f)) {
        perror("\nError reading");
      } else {
        printf("\n");
      }
      fclose(f);
//          printf("\nSleeping 200 microseconds\n");
//          microSpin(200);
    }
  }

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
        if (printFlag) rsif->print();
        printf("pgpWidget write returned 0x%x\n", rsif->_data);
      }
      break;
    case loopWriteCommand:
      if (count==0) count = 0xffffffff;
//      printf("\n");
      idx = 0;
      while (count--) {
        pgp->writeRegister(dest, addr, data, printFlag & 1, Pds::Pgp::PgpRSBits::Waiting);
        usleep(delay);
        rsif = pgp->read();
        if (rsif) {
          if (printFlag & 1) rsif->print();
          if (printFlag & 2) printf("\tcycle %u\n", idx++);
        }
      }
      break;
    case readCommand:
      ret = pgp->readRegister(dest, addr,0x2dbeef, &data, 1, printFlag);
      if (!ret) printf("pgpWidget read returned 0x%x\n", data);
      break;
    case dumpCommand:
      for (unsigned i=0; i<count; i++) {
        pgp->readRegister(dest, addr+i,0x2dbeef, &data);
        printf("\t%s0x%x - 0x%x\n", addr+i < 0x10 ? " " : "", addr+i, data);
      }
      break;
    case testCommand:
      for (unsigned i=0; i<count; i++) {
        pgp->writeRegister(dest, addr+i, i);
        pgp->readRegister(dest, addr+i,0x2dbeef, &data);
        printf("\t%16x - %16x %s\n", i, data, i == data ? "" : "<<--ERROR");
      }
      break;
    case readAsyncCommand:
      enum {BufferWords = 1<<24};
      Pds::Pgp::DataImportFrame* inFrame;
      PgpCardRx       pgpCardRx;
      pgpCardRx.model   = sizeof(&pgpCardRx);
      pgpCardRx.maxSize = BufferWords;
      pgpCardRx.data    = (uint32_t*)malloc(BufferWords);
      int readRet;
      while (keepGoing) {
        if ((readRet = ::read(fd, &pgpCardRx, sizeof(PgpCardRx))) >= 0) {
          if (writing && (readRet > 4)) {
            fwrite(pgpCardRx.data, sizeof(uint32_t), readRet, writeFile);
          } else if (debug & 1 || (readRet <= 4)) {
            inFrame = (Pds::Pgp::DataImportFrame*) pgpCardRx.data;
            printf("read returned %u, DataImportFrame lane(%u) vc(%u) pgpCardRx lane(%u), vc(%u)\n",
                readRet, inFrame->lane(), inFrame->vc(), pgpCardRx.pgpLane, pgpCardRx.pgpVc);
          } else {
            uint32_t* u32p = (uint32_t*) pgpCardRx.data;
            uint16_t* up = (uint16_t*) &u32p[9];
            unsigned readBytes = readRet * sizeof(uint32_t);
            for (unsigned i=0; i<(readRet == 4 ? 4 : 8); i++) {
              printf("%0x ", u32p[i]/*pgpCardRx.data[i]*/);
              readBytes -= sizeof(uint32_t);
            }
            up = (uint16_t*) &u32p[8];
            unsigned i=0;
            while ((readBytes>0) && (maxPrint > (8+i)) && (readRet != 4)) {
              printf("%0x ", up[i++]);
              readBytes -= sizeof(uint16_t);
            }
            printf("\n");
          }
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
