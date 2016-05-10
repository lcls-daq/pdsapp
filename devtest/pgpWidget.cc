#include "pdsapp/tools/PadMonServer.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/psddl/epixsampler.ddl.h"
#include "pds/pgp/Pgp.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pgpcard/PgpCardMod.h"
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

void printUsage(char* name) {
	printf( "Usage: %s [-h]  -P <pgpcardNumb> [-G] [-w dest,addr,data][-W dest,addr,data,count,delay][-r dest,addr][-d dest,addr,count][-t dest,addr,count][-R][-S detID,sharedMemoryTag,nclients][-a n,m,o][-o maxPrint][-s filename][-D <debug>] [-f <runTimeConfigName>][-p pf]\n"
			"    -h      Show usage\n"
			"    -P      Set pgpcard index number  (REQUIRED)\n"
			"                The format of the index number is a one byte number with the bottom nybble being\n"
			"                the index of the card and the top nybble being a port mask where one bit is for\n"
			"                each port, but a value of zero maps to 15 for compatibility with unmodified\n"
			"                applications that use the whole card\n"
			"    -G      Use if pgpcard is a G3 card\n"
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
			"    -S      Loop reading data until interrupted, resulting data will be written with the device type to the shared memory.  [Example: -S \"EpixSampler,0_1_DAQ\"\n"
			"    -o      Print out up to maxPrint words when reading data\n"
			"    -s      Save to file when reading data\n"
	    "    -a      Add more ports to G3 allocation, parameters are n first allocation and m second\n"
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

enum Commands{none, writeCommand,readCommand,readAsyncCommand,dumpCommand,testCommand,loopWriteCommand,addPortsCommand,numberOfCommands};

int main( int argc, char** argv )
{
	unsigned            pgpcard             = 0;
	unsigned            d                   = 0;
	char                g3[16]              = {""};
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
	bool                cardGiven           = false;
	unsigned            debug               = 0;
	unsigned            idx                 = 0;
	unsigned            portsToAdd[3]       = {0,0,0};
	int                 reti                 = SUCCESS;
	::signal( SIGINT, sigHandler );
	char                runTimeConfigname[256] = {""};
	char                writeFileName[256] = {""};
	PadMonServer*       shm                 = 0;
	PadMonServer::PadType typ               = PadMonServer::NumberOf;

	char*               endptr;
	extern char*        optarg;
	int c;
	while( ( c = getopt( argc, argv, "hP:Gw:W:D:r:d:RS:o:s:f:p:t:a:" ) ) != EOF ) {
		switch(c) {
		case 'P':
			pgpcard = strtoul(optarg, NULL, 0);
			cardGiven = true;
			break;
		case 'G':
			strcpy(g3, "G3");
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
		case 'a':
		  command = addPortsCommand;
		  portsToAdd[0] = strtoul(optarg  ,&endptr,0);
		  portsToAdd[1] = strtoul(endptr+1,&endptr,0);
		  portsToAdd[2] = strtoul(endptr+1,&endptr,0);
		  break;
		case 'R':
			command = readAsyncCommand;
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
		sprintf(devName, "/dev/pgpcard%s%u", g3, pgpcard);
	} else {
		sprintf(devName, "/dev/pgpcard%s_%u_%u", g3, pgpcard & 0xf, ports);
	}

	int fd = open( devName,  O_RDWR );
	if (debug & 1) printf("%s using %s\n", argv[0], devName);
	if (fd < 0) {
		sprintf(err, "%s opening %s failed", argv[0], devName);
		perror(err);
		// What else to do if the open fails?
		return 1;
	}

	unsigned limit = strlen(g3) ? 8 : 4;
	unsigned offset = 0;
	while ((((ports>>offset) & 1) == 0) && (offset < limit)) {
		offset += 1;
	}

	Pds::Pgp::Pgp::portOffset(offset);

	pgp = new Pds::Pgp::Pgp::Pgp(fd, debug != 0);
	dest = new Pds::Pgp::Destination::Destination(d);

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

	if (debug & 1) printf("%s destination %s\n", argv[0], dest->name());
	bool keepGoing = true;
	unsigned ret = 0;

	switch (command) {
	case none:
		printf("%s - no command given, exiting\n", argv[0]);
		return 0;
		break;
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
		ret = pgp->readRegister(dest, addr,0x2dbeef, &data, 1, printFlag);
		if (!ret) printf("%s read returned 0x%x\n", argv[0], data);
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
	case addPortsCommand:
	  printf("we allocated one to begin\n");
	  if (portsToAdd[0]) {
	    printf("Now adding %u ports\n", portsToAdd[0]);
	    reti = pgp->IoctlCommand(IOCTL_Add_More_Ports, portsToAdd[0]);
	  }
    if (portsToAdd[1] && reti == SUCCESS) {
      printf("Now adding %u ports\n", portsToAdd[1]);
      reti = pgp->IoctlCommand(IOCTL_Add_More_Ports, portsToAdd[1]);
    }
    if (portsToAdd[2] && reti == SUCCESS) {
      printf("Now adding %u ports\n", portsToAdd[2]);
      reti = pgp->IoctlCommand(IOCTL_Add_More_Ports, portsToAdd[2]);
    }
    if (reti == ERROR) {
      printf("We failed to add them all!!!\n");
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
					fwrite(pgpCardRx.data, sizeof(uint32_t), readRet, writeFile);
				} else if (debug & 1 || (readRet <= 4)) {
					printf("read returned %u, DataImportFrame lane(%u) vc(%u) pgpCardRx lane(%u), vc(%u)\n",
							readRet, inFrame->lane(), inFrame->vc(), pgpCardRx.pgpLane, pgpCardRx.pgpVc);
				} else {
					uint32_t* u32p = (uint32_t*) pgpCardRx.data;
					uint16_t* up = (uint16_t*) pgpCardRx.data;
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
					}
					unsigned readBytes = readRet * sizeof(uint32_t);
					if (printFlag) printf("[%d] ",readBytes);
					for (unsigned i=0; i<(readRet == 4 ? 4 : 8); i++) {
						if (printFlag) printf("%0x ", u32p[i]/*pgpCardRx.data[i]*/);
						readBytes -= sizeof(uint32_t);
					}
					up = (uint16_t*) &u32p[8];
					unsigned i=0;
					while ((readBytes>0) && (maxPrint > (8+i)) && (readRet != 4)) {
						if (printFlag) printf("%0x ", up[i++]);
						readBytes -= sizeof(uint16_t);
					}
					if (printFlag) printf("\n");
				}
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
