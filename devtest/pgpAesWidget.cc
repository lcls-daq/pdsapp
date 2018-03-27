#include "pdsapp/tools/PadMonServer.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/psddl/epixsampler.ddl.h"
#include "pds/pgp/Pgp.hh"
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

int printMyStatus (int s) {
   PgpInfo        info;
   PgpStatus      status;
   PciStatus      pciStatus;
   PgpEvrStatus   evrStatus;
   PgpEvrControl  evrControl;
   int            x;

   pgpGetInfo(s,&info);
   pgpGetPci(s,&pciStatus);
   unsigned low = (unsigned) (info.serial & 0xffffffff);
   unsigned high = (unsigned) ((info.serial>>32) & 0xffffffff);
   info.serial = (long long unsigned)high | ((long long unsigned)low<<32);

   printf("-------------- Card Info ------------------\n");
   printf("                 Type : 0x%.2x\n",info.type);
   printf("              Version : 0x%.8x\n",info.version);
   printf("               Serial : 0x%.16llx\n",(long long unsigned)(info.serial));
   printf("           BuildStamp : %s\n",info.buildStamp);
   printf("             LaneMask : 0x%.4x\n",info.laneMask);
   printf("            VcPerMask : 0x%.2x\n",info.vcPerMask);
   printf("              PgpRate : %i\n",info.pgpRate);
   printf("            PromPrgEn : %i\n",info.promPrgEn);

   printf("\n");
   printf("-------------- PCI Info -------------------\n");
   printf("           PciCommand : 0x%.4x\n",pciStatus.pciCommand);
   printf("            PciStatus : 0x%.4x\n",pciStatus.pciStatus);
   printf("          PciDCommand : 0x%.4x\n",pciStatus.pciDCommand);
   printf("           PciDStatus : 0x%.4x\n",pciStatus.pciDStatus);
   printf("          PciLCommand : 0x%.4x\n",pciStatus.pciLCommand);
   printf("           PciLStatus : 0x%.4x\n",pciStatus.pciLStatus);
   printf("         PciLinkState : 0x%x\n",pciStatus.pciLinkState);
   printf("          PciFunction : 0x%x\n",pciStatus.pciFunction);
   printf("            PciDevice : 0x%x\n",pciStatus.pciDevice);
   printf("               PciBus : 0x%.2x\n",pciStatus.pciBus);
   printf("             PciLanes : %i\n",pciStatus.pciLanes);

   for (x=0; x < 8; x++) {
      if ( ((1 << x) & info.laneMask) == 0 ) continue;

      pgpGetStatus(s,x,&status);
      pgpGetEvrStatus(s,x,&evrStatus);
      pgpGetEvrControl(s,x,&evrControl);

      printf("\n");
      printf("-------------- Lane %i --------------------\n",x);
      printf("             LoopBack : %i\n",status.loopBack);
      printf("         LocLinkReady : %i\n",status.locLinkReady);
      printf("         RemLinkReady : %i\n",status.remLinkReady);
      printf("              RxReady : %i\n",status.rxReady);
      printf("              TxReady : %i\n",status.txReady);
      printf("              RxCount : %i\n",status.rxCount);
      printf("           CellErrCnt : %i\n",status.cellErrCnt);
      printf("          LinkDownCnt : %i\n",status.linkDownCnt);
      printf("           LinkErrCnt : %i\n",status.linkErrCnt);
      printf("              FifoErr : %i\n",status.fifoErr);
      printf("              RemData : 0x%.2x\n",status.remData);
      printf("        RemBuffStatus : 0x%.2x\n",status.remBuffStatus);
      printf("           LinkErrors : %i\n",evrStatus.linkErrors);
      printf("               LinkUp : %i\n",evrStatus.linkUp);
      printf("            RunStatus : %i 1 = Running, 0 = Stopped\n",evrStatus.runStatus);    // 1 = Running, 0 = Stopped
      printf("          EvrFiducial : 0x%x\n",evrStatus.evrSeconds);
      printf("           RunCounter : 0x%x\n",evrStatus.runCounter);
      printf("        AcceptCounter : %i\n",evrStatus.acceptCounter);
      printf("            EvrEnable : %i Global flag\n",evrControl.evrEnable);     // Global flag
      printf("          LaneRunMask : %i 0 = Run trigger enable\n",evrControl.laneRunMask);   // 1 = Run trigger enable
      printf("            EvrLaneEn : %i 1 = Start, 0 = Stop\n",evrControl.evrSyncEn);     // 1 = Start, 0 = Stop
      printf("           EvrSyncSel : %i 0 = async, 1 = sync for start/stop\n",evrControl.evrSyncSel);    // 0 = async, 1 = sync for start/stop
      printf("           HeaderMask : 0x%x 1 = Enable header data checking, one bit per VC (4 bits)\n",evrControl.headerMask);    // 1 = Enable header data checking, one bit per VC (4 bits)
      printf("          EvrSyncWord : 0x%x fiducial to transition start stop\n",evrControl.evrSyncWord);   // fiducial to transition start stop
      printf("              RunCode : %i Run code\n",evrControl.runCode);       // Run code
      printf("             RunDelay : %i Run delay\n",evrControl.runDelay);      // Run delay
      printf("           AcceptCode : %i DAQ code\n",evrControl.acceptCode);    // Accept code
      printf("          AcceptDelay : %i DAQ delay\n",evrControl.acceptDelay);   // Accept delay
   }
   return(0);
}


void printUsage(char* name) {
  printf( "Usage: %s [-h]  -P <cardNumb,portVcNumb,offset> [-w addr,data][-W addr,data,count,delay][-r addr][-d addr,count][-t addr,count][-R][-S detID,sharedMemoryTag,nclients][-o maxPrint][-s filename][-D <debug>][-m][M][-f <runTimeConfigName>][-X <addr,count>][x][-p pf]\n"
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
      "    -d      Dump count registers starting at addr\n"
      "    -m      Monitor testing -  write enable and then loop reading\n"
      "    -M      Disable monitor output, write disable"
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
      "    -p      set print flag to value given\n",
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
  DmaWriteData           tx;
  ::signal( SIGINT, sigHandler );

  char*               endptr;
  extern char*        optarg;
  int c;
  while( ( c = getopt( argc, argv, "hP:w:W:D:r:d:RS:o:s:f:p:t:mMX:x" ) ) != EOF ) {
    switch(c) {
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
  pgp = new Pds::Pgp::Pgp(fd, debug != 0);
  dest = new Pds::Pgp::Destination(lvcNumb);
  pgp->allocateVC(1<<(lvcNumb&3), 1<<(lvcNumb>>2));

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
      pgp->writeRegister(dest, addr, data, printFlag, Pds::Pgp::PgpRSBits::Waiting);
      rsif = pgp->read();
      if (rsif) {
        if (printFlag) rsif->print();
        printf("%s write returned 0x%x\n", argv[0], rsif->_data);
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
      ret = pgp->readRegister(dest, addr,0x2dbeef, &data, 1, printFlag!=0);
      if (!ret) printf("%s read returned 0x%x\n", argv[0], data);
      break;
    case dumpCommand:
      printf("%s reading %u registers at %x\n", argv[0], count, (unsigned)addr);
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
    case printStatus:
      printMyStatus(fd);
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
      tx.is32   = sizeof(&tx) == 4;
      tx.dest = 3 + (4*pgp->portOffset());
      tx.flags = 0;
      tx.size = sizeof(tx);
      tx.data = (unsigned long long)&val;
      printf("monitorTest write %d to lane %d vc %d, offset %d\n", val, tx.dest>>2, tx.dest&3, pgp->portOffset());
      write(fd, &tx, sizeof(tx));
      break;
    case monitorTest:
      val = one;
      tx.is32   = sizeof(&tx) == 4;
      tx.dest = 3 + (4*pgp->portOffset());
      tx.flags = 0;
      tx.size = sizeof(tx);
      tx.data = (unsigned long long)&val;
      printf("monitorTest write %d to lane %d vc %d, offset %d\n", val, tx.dest>>2, tx.dest&3, pgp->portOffset());
      write(fd, &tx, sizeof(tx));
      //fall through to readAsync ...
    case readAsyncCommand:
      enum {BufferWords = 1<<24};
      Pds::Pgp::DataImportFrame* inFrame;
      DmaReadData       pgpCardRx;
      pgpCardRx.data    = (uint64_t)malloc(BufferWords);
      pgpCardRx.dest    = dest->dest();
      pgpCardRx.flags   = 0;
      pgpCardRx.index   = 0;
      pgpCardRx.error   = 0;
      pgpCardRx.size    = BufferWords*4;
      pgpCardRx.is32    = sizeof(&pgpCardRx) == 4;
      int readRet;
      while (keepGoing) {
        if ((readRet = ::read(fd, &pgpCardRx, sizeof(DmaReadData))) > 0) {
          inFrame = (Pds::Pgp::DataImportFrame*) pgpCardRx.data;
          if (shm) {
            switch(typ) {
              case PadMonServer::Epix:
                if (readRet > 4000) // ignore configure returns
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
              printf("Temperature 1 [C]: %f\n", (double)((int)i32p[8])/100);
              printf("Temperature 2 [C]: %f\n", (double)((int)i32p[9])/100);
              printf("Humidity [%%]: %f\n", (double)(i32p[10])/100);
              printf("ASIC analog current [mA]: %d\n", (u32p[11]));
              printf("ASIC digital current [mA]: %d\n", (u32p[12]));
              printf("ASIC guard ring current [uA]: %d\n", (u32p[13]));
              printf("Analog input voltage [mV]: %d\n", (u32p[14]));
              printf("Digital input voltage [mV]: %d\n\n", (u32p[15]));
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
