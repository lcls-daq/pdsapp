#include "pdsapp/tools/PadMonServer.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/psddl/epixsampler.ddl.h"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/SrpV3.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/DataImportFrame.hh"
#include <PgpDriver.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <new>

FILE*               writeFile           = 0;
bool                writing             = false;

void sigHandler( int signal ) {
  psignal( signal, "Signal received by pgpWidget");
  if (writing) fclose(writeFile);
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}

static double getThermistorTemp(const uint16_t x)
{
  if (x==0) return 0.;
  double u = double(x)/16383.0 * 2.5;
  double i = u / 100000;
  double r = (2.5 - u)/i;
  double l = log(r/10000);
  double t = 1.0 / (3.3538646E-03 + 2.5654090E-04 * l + 1.9243889E-06 * (l*l) + 1.0969244E-07 * (l*l*l));
  return t - 273.15;
}

using namespace Pds;

Pgp::Destination* dest;
Pgp::Pgp* pgp;
Pds::Pgp::RegisterSlaveImportFrame* rsif;

void printUsage(char* name) {
  printf( "Usage: %s [-h]  -P <cardNumb,portVcNumb,offset> [-w addr,data][-W addr,data,count,delay][-r addr][-d addr,count][-t addr,count][-R][-S detID,sharedMemoryTag,nclients][-o maxPrint][-s filename][-D <debug>][-m][M][-f <runTimeConfigName>][-X <addr,count>][x][-p pf][-3][-K]\n"
      "    -h      Show usage\n"
      "    -P      Set pgpcard card number and port/vc\n"
      "                 The first number is just the index of the card, default zero.  The second number,\n"
      "                 is the number of the destination calculated by multiplying the lane number\n"
      "                 by four and adding the vc number.  The third number is the pgp offset sent to"
      "                 the pgp class and not normally seen inside the application\n"
      "                 In this scheme, the second number should not be larger than 3 for a one lane\n"
      "                 sensor or 12 for a two lane sensor and the third number should not be larger than\n"
      "                 7 for a one lane sensor or 6 for a two lane sensor.\n"
      "    -w      Write register to destination, reply will be send to standard output\n"
      "                The format of the paraemeters are: 'addr data'\n"
      "                where addr and data are 32 bit unsigned integers\n"
      "    -W      Loop writing register to destination count times, unless count = 0 which means forever\n"
      "                 delay is time between packets sent in microseconds\n"
      "                 other parameters are the same as the write command above\n"
      "    -r      Read register from destination, resulting data, is written to the standard output\n"
      "                The format of the parameters is: 'addr'\n"
      "                where addr is a 32 bit unsigned integer\n"
      "    -t      Test registers, loop from addr counting up count times\n"
      "    -T      Dump memory mapped Tx buffers\n"
      "    -d      Dump count registers starting at addr\n"
      "    -m      Monitor testing -  write enable and then loop reading\n"
      "    -M      Disable monitor output, write disable\n"
      "    -R      Loop reading data until interrupted, resulting data will be written to the standard output.\n"
      "    -S      Loop reading data until interrupted, resulting data will be written with the device type to the shared memory.  [Example: -S \"EpixSampler,0_1_DAQ\"\n"
      "    -o      Print out up to maxPrint words when reading data\n"
      "    -s      Save to file when reading data\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          print out progress\n"
      "    -f      set run time config file name\n"
      "                The format of the file consists of lines: 'Dest Addr Data'\n"
      "                where Addr and Data are 32 bit unsigned integers, but the Dest is an\n"
      "                upt to five bit field where the bottom two bits are VC number and the top two or three are Lane number.\n"
      "                NB, note that this dest field stays compatible with old files and is converted to new destination\n"
      "                format by the software.\n"
      "    -x      print status\n"
      "    -X      print pgpcard registers at addr for count registers\n"
      "    -O      set Pgp offset, default is zero\n"
      "    -p      set print flag to value given\n"
      "    -3      Use SLAC register protocol version 3\n"
      "    -K      flag to indicate the card is kcu1500/datadev card\n",
      name
  );
}

enum Commands{none, writeCommand, readCommand, readAsyncCommand,
  dumpCommand, testCommand, loopWriteCommand,
  monitorTest, disMonitorTest, printStatus, printRegisters, numberOfCommands};

int main( int argc, char** argv ) {
  unsigned            pgpcard             = 0;
  uint32_t            data                = 0xdead;
  unsigned            command             = none;
  unsigned            addr                = 0;
  unsigned            count               = 0;
  unsigned            delay               = 1000;
  unsigned            printFlag           = 0;
  unsigned            maxPrint            = 0;
  unsigned            lastAcq             = 0;
  unsigned            thisAcq             = 0;
  unsigned*           errHisto            = 0;
  unsigned            counter             = 0;
  errHisto = (unsigned*) calloc(100, sizeof(unsigned));
  unsigned			lastOpCode          = 0;
  unsigned            thisOpCode          = 0;
  unsigned*			ocerrHisto          = 0;
  unsigned            occounter           = 0;
  ocerrHisto = (unsigned*) calloc(100, sizeof(unsigned));
  unsigned            slastAcq            = 0;
  unsigned            sthisAcq            = 0;
  unsigned*           serrHisto           = 0;
  unsigned            scounter            = 0;
  serrHisto = (unsigned*) calloc(100, sizeof(unsigned));
  //  bool                found               = false;
  unsigned            debug               = 0;
  unsigned            idx                 = 0;
  unsigned            val                 = 0;
  unsigned            one                 = 1;
  unsigned            zero                = 0;
  unsigned            lvcNumb             = 1;
  //  unsigned            lane                = 0;
  unsigned            offset              = 0;
  char                runTimeConfigname[256] = {""};
  char                writeFileName[256] = {""};
  char                devName[128] = {""};
  PadMonServer*       shm                 = 0;
  PadMonServer::PadType typ               = PadMonServer::NumberOf;
  unsigned            dumpTxBuffers = 0;
  DmaWriteData           tx;
  bool                srpV3 = false;
  bool                datadev = false;
  ::signal( SIGINT, sigHandler );

  char*               endptr;
  extern char*        optarg;
  int c;
  while( ( c = getopt( argc, argv, "hP:w:W:D:r:d:RS:o:s:f:p:t:T:mMX:x3K" ) ) != EOF ) {
    switch(c) {
      case 'T': dumpTxBuffers = strtoul(optarg, &endptr,0); break;
      case 'P':
        pgpcard = strtoul(optarg  ,&endptr,0);
        lvcNumb = strtoul(endptr+1,&endptr,0);
        offset = strtoul(endptr+1,&endptr,0);
        printf("Pgp offset will be set to %d\n", offset);
        break;
      case 'D':
        debug = strtoul(optarg, NULL, 0);
        break;
      case 'f':
        strcpy(runTimeConfigname, optarg);
        break;
      case 'w':
        command = writeCommand;
        addr = strtoul(optarg,&endptr,0);
        data = strtoul(endptr+1,&endptr,0);
        break;
      case 'W':
        command = loopWriteCommand;
        addr = strtoul(optarg,&endptr,0);
        data = strtoul(endptr+1,&endptr,0);
        count = strtoul(endptr+1,&endptr,0);
        delay = strtoul(endptr+1,&endptr,0);
        //        printf("loopWriteCommand %u,%u,0x%x,%u\n", d, addr, data, count);
        break;
      case 'r':
        command = readCommand;
        addr = strtoul(optarg  ,&endptr,0);
        if (debug & 1) printf("\t read at 0x%x\n", addr);
        break;
      case 'd':
        command = dumpCommand;
        addr = strtoul(optarg,&endptr,0);
        count = strtoul(endptr+1,&endptr,0);
        printf("\t dump %u at %u\n", count, addr);
        break;
      case 't':
        command = testCommand;
        addr = strtoul(optarg,&endptr,0);
        count = strtoul(endptr+1,&endptr,0);
        break;
      case 'm':
        command = monitorTest;
        break;
      case 'M':
        command = disMonitorTest;
        break;
      case 'R':
        command = readAsyncCommand;
        break;
      case 'x':
        command = printStatus;
        break;
      case 'X':
        command = printRegisters           ;
        addr = strtoul(optarg,&endptr,0);
        count = strtoul(endptr+1,&endptr,0);
       break;
      case 'S':
        command = readAsyncCommand;
        { char* dev  = strtok(optarg,",");
        if      (strcmp(dev,"Epix")==0)        typ = PadMonServer::Epix;
        else if (strcmp(dev,"EpixSampler")==0) typ = PadMonServer::EpixSampler;
        else { printf("device type %s not understood\n",dev); return -1; }
        char* tag  = strtok(NULL,",");
        char* nclstr = strtok(NULL,",");
        unsigned nclients=1;
        if (nclstr)
          nclients=strtoul(nclstr,NULL,0);
        shm = new PadMonServer(typ, tag, nclients);
        switch(typ) {
#if 1
          case PadMonServer::Epix:
          { const unsigned AsicsPerRow=2;
          const unsigned AsicsPerColumn=2;
          const unsigned Asics=AsicsPerRow*AsicsPerColumn;
          const unsigned Rows   =4*88;
          const unsigned Columns=4*96;
          Epix::AsicConfigV1 asics[Asics];
          uint32_t* testarray = new uint32_t[Asics*Rows*(Columns+31)/32];
          memset(testarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
          uint32_t* maskarray = new uint32_t[Asics*Rows*(Columns+31)/32];
          memset(maskarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
          unsigned asicMask=0xf;

          char* p = new char[0x1000000];
          shm->configure(*new(p)Epix::ConfigV1( 0, 1, 0, 1, 0,
              0, 0, 0, 0, 0,
              0, 0, 0, 0, 0,
              0, 0, 0, 0, 0,
              0, 0, 0, 0, 0,
              1, 0, 0, 0, 0,
              0, 0, 0, 0, 0,
              AsicsPerRow, AsicsPerColumn,
              Rows, Columns,
              0x200000, // 200MHz
              asicMask,
              asics, testarray, maskarray ) );
          delete[] p;
          } break;
#endif
          case PadMonServer::EpixSampler:
          { const unsigned Channels = 8;
          const unsigned Samples  = 8192;
          const unsigned BaseClkFreq = 100000000;
          shm->configure(EpixSampler::ConfigV1( 0, 0, 0, 0, 1,
              0, 0, 0, 0, 0,
              Channels, Samples,
              BaseClkFreq, 0 ) );
          } break;
          default:
            break;
        } }
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
      case '3':
        srpV3 = true;
        break;
      case 'K':
        datadev = true;
        break;
      case 'h':
        printUsage(argv[0]);
        return 0;
        break;
      default:
        printf("Error: Option could not be parsed, or is not supported!\n");
        printUsage(argv[0]);
        return 0;
        break;
    }
  }

  char err[128];
  if (datadev)
    sprintf(devName, "/dev/datadev_%u",pgpcard);
  else
    sprintf(devName, "/dev/pgpcard_%u",pgpcard);

  int fd = open( devName,  O_RDWR );
  printf("%s using %s\n", argv[0], devName);
  if (fd < 0) {
    sprintf(err, "%s opening %s failed", argv[0], devName);
    perror(err);
    // What else to do if the open fails?
    return 1;
  }

  Pds::Pgp::Pgp::portOffset(offset);
  pgp = new Pds::Pgp::Pgp(true, fd, debug != 0);
  dest = new Pds::Pgp::Destination(lvcNumb);
  pgp->allocateVC(1<<(lvcNumb&3), 1<<(lvcNumb>>2));

  if (dumpTxBuffers) {
    // memory map
    unsigned txCount, txSize;
    void** txBuffers = dmaMapDma(fd, &txCount, &txSize);
    for(unsigned i=0; i<txCount; i++) {
      printf("[%02u]",i);
      for(unsigned j=0; j<dumpTxBuffers; j++)
        printf(" %08x",reinterpret_cast<uint32_t*>(txBuffers[i])[j]);
      printf("\n");
    }
  }

  if (debug & 1) printf("Destination %s Offset %u\n", dest->name(), offset);

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

  if (strlen(runTimeConfigname)) {
    FILE* f;
    Pgp::Destination _d;
    unsigned maxCount = 1024;
    //    char path[240];
    //    char* home = getenv("HOME");
    //    sprintf(path,"%s/%s",home, runTimeConfigname);
    const char* path = runTimeConfigname;
    printf("Configuring from %s\n", path);
    f = fopen (path, "r");
    if (!f) {
      char s[200];
      sprintf(s, "Could not open %s ", path);
      perror(s);
    } else {
      unsigned myi = 0;
      unsigned dest, addr, data, lane, vc, odest;
      while (fscanf(f, "%x %x %x", &odest, &addr, &data) && !feof(f) && myi++ < maxCount) {
        lane = (odest&0xc)>>2; vc = odest&3;
        dest = (((lane+offset)&7)<<2) + vc;  _d.dest(dest);
        printf("\nConfig from file, dest %s, addr 0x%x, data 0x%x ", _d.name(), addr, data);
        if(pgp->writeRegister(&_d, addr, data)) {
          printf("\npgpwidget writing config failed on old style dest %u address 0x%x\n", odest, addr);
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

  if (debug & 1) printf("%s destination %s\n", argv[0], dest->name());
  bool keepGoing = true;
  unsigned ret = 0;

  switch (command) {
    case none:
      printf("%s - no command given, exiting\n", argv[0]);
      return 0;
      break;
//    case printStatus:
//      pgp->status()->print();
//      break;
    case writeCommand:
      if (srpV3) {
        Pds::Pgp::SrpV3::Protocol* proto = new Pds::Pgp::SrpV3::Protocol(fd, offset);
        proto->writeRegister(dest, addr, data);
      }
      else {
        pgp->writeRegister(dest, addr, data, printFlag, Pds::Pgp::PgpRSBits::Waiting);
        rsif = pgp->read();
        if (rsif) {
          if (printFlag) rsif->print();
          printf("%s write returned 0x%x\n", argv[0], rsif->_data);
        }
      }
      break;
    case loopWriteCommand:
      if (count==0) count = 0xffffffff;
      //      printf("\n");
      idx = 0;
      while (count--) {
        unsigned v;
        if (srpV3) {
          Pds::Pgp::SrpV3::Protocol* proto = new Pds::Pgp::SrpV3::Protocol(fd, offset);
          proto->writeRegister(dest, addr, data);
          ret = proto->readRegister(dest, addr, 0, &v);
        }
        else {
          pgp->writeRegister(dest, addr, data, printFlag & 1, Pds::Pgp::PgpRSBits::Waiting);
          usleep(delay);
          rsif = pgp->read();
          if (rsif) {
            if (printFlag & 1) rsif->print();
            if (printFlag & 2) printf("\tcycle %u\n", idx++);
          }
          ret = pgp->readRegister(dest, addr, 0x2dbeef, &v, 1, printFlag!=0);
        }
        if (ret) printf("Read return 0x%x\n", ret);
        if (v != data)
          printf("Wrote %x  read %x\n", data, v);
        data++;
      }
      break;
    case readCommand:
      if (srpV3) {
        Pds::Pgp::SrpV3::Protocol* proto = new Pds::Pgp::SrpV3::Protocol(fd, offset);
        ret = proto->readRegister(dest, addr,0x2dbeef, &data);
      }
      else
        ret = pgp->readRegister(dest, addr,0x2dbeef, &data, 1, printFlag!=0);
      if (!ret) printf("%s read returned 0x%x\n", argv[0], data);
      break;
    case dumpCommand:
      printf("%s reading %u registers at %x\n", argv[0], count, (unsigned)addr);
      if (srpV3) {
        Pds::Pgp::SrpV3::Protocol* proto = new Pds::Pgp::SrpV3::Protocol(fd, offset);
        for (unsigned i=0; i<count; i++) {
          proto->readRegister(dest, addr+i, 0x2dbeef+i, &data);
          printf("\t%s0x%x - 0x%x\n", addr+i < 0x10 ? " " : "", addr+i, data);
        }
      } else {
        for (unsigned i=0; i<count; i++) {
          pgp->readRegister(dest, addr+i, 0x2dbeef+i, &data);
          printf("\t%s0x%x - 0x%x\n", addr+i < 0x10 ? " " : "", addr+i, data);
        }
      }
      break;
    case testCommand:
      if (srpV3) {
        Pds::Pgp::SrpV3::Protocol* proto = new Pds::Pgp::SrpV3::Protocol(fd, offset);
        for (unsigned i=0; i<count; i++) {
          proto->writeRegister(dest, addr+i, i);
          proto->readRegister(dest, addr+i,0x2dbeef, &data);
          printf("\t%16x - %16x %s\n", i, data, i == data ? "" : "<<--ERROR");
        }
      } else {
        for (unsigned i=0; i<count; i++) {
          pgp->writeRegister(dest, addr+i, i);
          pgp->readRegister(dest, addr+i,0x2dbeef, &data);
          printf("\t%16x - %16x %s\n", i, data, i == data ? "" : "<<--ERROR");
        }
      }
      break;
    case printStatus:
      pgp->printStatus();
      break;
    case printRegisters           :
      printf("PGPcard registers:\n");
      for (unsigned i=addr; i<addr+count; i++) {
        unsigned tmp;
        ssize_t ret;
        ret = dmaReadRegister(fd, i, &tmp);
        if (ret<0) perror("dmaReadRegister: ");
        else printf("\t%3u\t0x%x\t%d\n", i, tmp, (int)ret);
      }
      break;
    case disMonitorTest:
      val = zero;
      if (srpV3) {
        unsigned cmd[] = {0, val, 0, 0};
        tx.is32   = sizeof(&tx) == 4;
        tx.dest = 3 + (4*pgp->portOffset());
        tx.index = 0;
        tx.flags = 0;
        tx.size = sizeof(cmd);
        tx.data = (unsigned long long)&cmd;
      } else {
        tx.is32   = sizeof(&tx) == 4;
        tx.dest = 3 + (4*pgp->portOffset());
        tx.index = 0;
        tx.flags = 0;
        tx.size = sizeof(val);
        tx.data = (unsigned long long)&val;
      }
      printf("monitorTest write %d to lane %d vc %d, offset %d\n", val, tx.dest>>2, tx.dest&3, pgp->portOffset());
      write(fd, &tx, sizeof(tx));
      break;
    case monitorTest:
      val = one;
      if (srpV3) {
        unsigned cmd[] = {0, val, 0, 0};
        tx.is32   = sizeof(&tx) == 4;
        tx.dest = 3 + (4*pgp->portOffset());
        tx.flags = 0;
        tx.index = 0;
        tx.size = sizeof(cmd);
        tx.data = (unsigned long long)&cmd;
      } else {
        tx.is32   = sizeof(&tx) == 4;
        tx.dest = 3 + (4*pgp->portOffset());
        tx.flags = 0;
        tx.index = 0;
        tx.size = sizeof(val);
        tx.data = (unsigned long long)&val;
      }
      printf("monitorTest write %d to lane %d vc %d, offset %d\n", val, tx.dest>>2, tx.dest&3, pgp->portOffset());
      write(fd, &tx, sizeof(tx));
      //fall through to readAsync ...
    case readAsyncCommand:
      enum {BufferWords = 1<<24};
      Pds::Pgp::DataImportFrame* inFrame;
      DmaReadData       pgpCardRx;
      pgpCardRx.data    = (uint64_t)malloc(BufferWords);
      pgpCardRx.dest    = 0;
      pgpCardRx.ret     = 0;
      pgpCardRx.flags   = 0;
      pgpCardRx.index   = 0;
      pgpCardRx.error   = 0;
      pgpCardRx.size    = BufferWords*4;
      pgpCardRx.is32    = sizeof(&pgpCardRx) == 4;
      int readRet;
      while (keepGoing) {
        if ((readRet = ::read(fd, &pgpCardRx, sizeof(DmaReadData))) > 0) {
          readRet = pgpCardRx.ret / sizeof(uint32_t);
          inFrame = (Pds::Pgp::DataImportFrame*) pgpCardRx.data;
          if (shm) {
            switch(typ) {
              case PadMonServer::Epix:
                if (readRet > 1000) // ignore configure returns
                  shm->event(*reinterpret_cast<Epix::ElementV1*>(pgpCardRx.data)); break;
              case PadMonServer::EpixSampler:
                shm->event(*reinterpret_cast<EpixSampler::ElementV1*>(pgpCardRx.data)); break;
              default:
                break;
            }
          } else if (writing && (readRet > 4)) {
            fwrite((const void *)pgpCardRx.data, sizeof(uint32_t), readRet, writeFile);
          } else if ((debug & 1) || (readRet <= 4)) {
            printf("read returned %u, DataImportFrame lane(%u) vc(%u) pgpCardRx lane(%u), vc(%u)\n",
                readRet, inFrame->lane(), inFrame->vc(),
                pgpGetLane(pgpCardRx.dest)-Pds::Pgp::Pgp::portOffset(), pgpGetVc(pgpCardRx.dest));
          } else {
            uint32_t* u32p = (uint32_t*) pgpCardRx.data;
            int*      i32p = (int*) pgpCardRx.data;
            uint16_t* u16p = (uint16_t*) pgpCardRx.data;
            unsigned diff;
            if (inFrame->vc()==0) { // a data frame
              if (lastAcq) {
                thisAcq = inFrame->acqCount();
                if (thisAcq > lastAcq) {
                  if ((diff = thisAcq - lastAcq) > 1) {
                    errHisto[diff] += 1;
                    //									printf("count %d, last %d, this%d, diff %d\n", counter, lastAcq, thisAcq, diff);
                  }
                  lastAcq = thisAcq;
                } else {
                  lastAcq = inFrame->acqCount();
                }
                if (((++counter)%10000)==0){
                  printf("ERROR GAPS [%d] ", counter);
                  for (unsigned y=0; y<100; y++) if (errHisto[y]) printf("(%d)%d ", y-1, errHisto[y]);
                  printf("\n");
                }
              } else {
                lastAcq = inFrame->acqCount();
              }
              if (lastOpCode) {
                thisOpCode = inFrame->opCode();
                if (thisOpCode > lastOpCode) {
                  if ((diff = thisOpCode - lastOpCode) > 1) {
                    ocerrHisto[diff] += 1;
                    //									printf("count %d, last %d, this%d, diff %d\n", counter, lastOpCode, thisOpCode, diff);
                  }
                  lastOpCode = thisOpCode;
                } else {
                  if (thisOpCode==lastOpCode) {
                    ocerrHisto[0] += 1;
                  } else {
                    if ((diff = thisOpCode + 256 - lastOpCode) > 1) {
                      ocerrHisto[diff] += 1;
                      //									printf("count %d, last %d, this%d, diff %d\n", counter, lastOpCode, thisOpCode, diff);
                    }
                  }
                  lastOpCode = inFrame->opCode();
                }
                if (((++occounter)%10000)==0){
                  printf("OpCode GAPS [%d] ", occounter);
                  for (unsigned y=0; y<100; y++) if (ocerrHisto[y]) printf("(%d)%d ", y==0 ? y : y-1, ocerrHisto[y]);
                  printf("\n");
                }
              } else {
                lastOpCode = inFrame->opCode();
              }
            } else if (inFrame->vc()==2) { // a scope frame
              if (slastAcq) {
                sthisAcq = inFrame->acqCount();
                if (sthisAcq > slastAcq) {
                  if ((diff = sthisAcq - slastAcq) > 1) {
                    serrHisto[diff] += 1;
                    //									printf("count %d, last %d, this%d, diff %d\n", counter, lastAcq, thisAcq, diff);
                  }
                  slastAcq = sthisAcq;
                } else {
                  slastAcq = inFrame->acqCount();
                }
                if (((++scounter)%10000)==0){
                  printf("SCOPE GAPS [%d] ", scounter);
                  for (unsigned y=0; y<100; y++) if (serrHisto[y]) printf("(%d)%d ", y-1, serrHisto[y]);
                  printf("\n");
                }
              } else {
                slastAcq = inFrame->acqCount();
              }
            } else if ((inFrame->vc()==3) && (command == monitorTest)) {
              if (srpV3) {
                printf("Sht31 Humidity [%%]: %f\n", double(u16p[16])/65535.0 * 100);
                printf("Sht31 Temperature [C]: %f\n", double(u16p[17])/65535.0 * 175 - 45);
                printf("NctLoc Temperature [C]: %f\n", double(u16p[18]&0xff));
                printf("NctFpga Temperature [C]: %f\n", double(u16p[19]>>8)+double(u16p[19]&0xc0)/256.);
                printf("A0_2V5 Current [mA]: %f\n", double(u16p[20])/16383.0*2.5/330.*1.e6);
                printf("A1_2V5 Current [mA]: %f\n", double(u16p[21])/16383.0*2.5/330.*1.e6);
                printf("A2_2V5 Current [mA]: %f\n", double(u16p[22])/16383.0*2.5/330.*1.e6);
                printf("A3_2V5 Current [mA]: %f\n", double(u16p[23])/16383.0*2.5/330.*1.e6);
                printf("D0_2V5 Current [mA]: %f\n", double(u16p[24])/16383.0*2.5/330.*0.5e6);
                printf("D1_2V5 Current [mA]: %f\n", double(u16p[25])/16383.0*2.5/330.*0.5e6);
                printf("Thermistor 0 Temperature [C]: %f\n", getThermistorTemp(u16p[26]));
                printf("Thermistor 1 Temperature [C]: %f\n", getThermistorTemp(u16p[27]));
                printf("PwrDig Current [A]: %f\n", double(u16p[28])*0.1024/4095/0.02);
                printf("PwrDig Voltage [V]: %f\n", double(u16p[29])*102.4/4095);
                printf("PwrDig Temperature [C]: %f\n", double(u16p[30])*2.048/4095*(130/(0.882-1.951)) + (0.882/0.0082+100));
                printf("PwrAna Current [A]: %f\n", double(u16p[31])*0.1024/4095/0.02);
                printf("PwrAna Voltage [V]: %f\n", double(u16p[32])*102.4/4095);
                printf("PwrAna Temperature [C]: %f\n", double(u16p[33])*2.048/4095*(130/(0.882-1.951)) + (0.882/0.0082+100));
                printf("A0_2V5_H Temperature [C]: %f\n", double(u16p[34])*1.65/65535*100);
                printf("A0_2V5_L Temperature [C]: %f\n", double(u16p[35])*1.65/65535*100);
                printf("A1_2V5_H Temperature [C]: %f\n", double(u16p[36])*1.65/65535*100);
                printf("A1_2V5_L Temperature [C]: %f\n", double(u16p[37])*1.65/65535*100);
                printf("A2_2V5_H Temperature [C]: %f\n", double(u16p[38])*1.65/65535*100);
                printf("A2_2V5_L Temperature [C]: %f\n", double(u16p[39])*1.65/65535*100);
                printf("A3_2V5_H Temperature [C]: %f\n", double(u16p[40])*1.65/65535*100);
                printf("A3_2V5_L Temperature [C]: %f\n", double(u16p[41])*1.65/65535*100);
                printf("D0_2V5 Temperature [C]: %f\n", double(u16p[42])*1.65/65535*100);
                printf("D1_2V5 Temperature [C]: %f\n", double(u16p[43])*1.65/65535*100);
                printf("A0_1V8 Temperature [C]: %f\n", double(u16p[44])*1.65/65535*100);
                printf("A1_1V8 Temperature [C]: %f\n", double(u16p[45])*1.65/65535*100);
                printf("A2_1V8 Temperature [C]: %f\n", double(u16p[46])*1.65/65535*100);
                printf("PcbAna Temperature 0 [C]: %f\n", double(u16p[47])*1.65/65535*(130/0.882-1.951)+(0.882/0.0082+100));
                printf("PcbAna Temperature 1 [C]: %f\n", double(u16p[48])*1.65/65535*(130/0.882-1.951)+(0.882/0.0082+100));
                printf("PcbAna Temperature 2 [C]: %f\n", double(u16p[49])*1.65/65535*(130/0.882-1.951)+(0.882/0.0082+100));
                printf("TrOpt Temperature [C]: %f\n", double(u16p[50])/256);
                printf("TrOpt Voltage [V]: %f\n", double(u16p[51])*0.0001);
                printf("TrOpt TxPwr [uW]: %f\n", double(u16p[52])*0.1);
                printf("TrOpt RxPwr [uW]: %f\n", double(u16p[53])*0.1);
              } else {
                printf("Temperature 1 [C]: %f\n", (double)((int)i32p[8])/100);
                printf("Temperature 2 [C]: %f\n", (double)((int)i32p[9])/100);
                printf("Humidity [%%]: %f\n", (double)(i32p[10])/100);
                printf("ASIC analog current [mA]: %d\n", (u32p[11]));
                printf("ASIC digital current [mA]: %d\n", (u32p[12]));
                printf("ASIC guard ring current [uA]: %d\n", (u32p[13]));
                printf("Analog input voltage [mV]: %d\n", (u32p[14]));
                printf("Digital input voltage [mV]: %d\n\n", (u32p[15]));
              }
            } else {
              unsigned readBytes = readRet;
              if (printFlag) printf("[%d] ",readBytes);
              for (unsigned i=0; i<((readRet/sizeof(uint32_t)) == 4 ? 4 : 8); i++) {
                if (printFlag) printf("%0x ", u32p[i]/*pgpCardRx.data[i]*/);
                readBytes -= sizeof(uint32_t);
              }
              u16p = (uint16_t*) &u32p[8];
              unsigned i=0;
              while ((readBytes>0) && (maxPrint > (8+i)) && (readRet != 4)) {
                if (printFlag) printf("%0x ", u16p[i++]);
                readBytes -= sizeof(uint16_t);
              }
              if (printFlag) printf("\n");
            }
          }
        } else if (readRet == 0) {
          ;
        } else {
          perror("pgpWidget Async reading error");
          keepGoing = false;
        }
      }
      break;
    default:
      printf("%s - unknown command, exiting\n", argv[0]);
      break;
  }

  return 0;
}
