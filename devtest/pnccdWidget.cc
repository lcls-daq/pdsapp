//#include "pgpcard/PgpCardMod.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <string>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <new>
#include "pds/xtc/EvrDatagram.hh"
#include <sys/ioctl.h>
#include <time.h>
FILE*               writeFile           = 0;
unsigned            laneMask            = 0xf;
unsigned            laneErrors          = 0;
bool                writing             = false;
bool                flipFlag            = false;
bool                includeElementID    = true;
unsigned            printModuloBase     = 1000;
uint16_t foobar[4] = {1028, 1030, 1032, 1026};
enum {pnCCDMagicWord=0x1c000300,pnCCDFrameSize=131075}; // ((1<<19)/4) + 4 - 1
enum {pnCCDPayloadSize=524288,numberOfBuffers=32}; // 1<<19

// Normal Write commmand
#define IOCTL_Normal_Write         0
#define IOCTL_Write_Scratch      0xD

typedef struct {

   unsigned  model; // large=8, small=4
   unsigned  cmd; // ioctl commands
   unsigned* data;
   // Lane & VC
   unsigned  pgpLane;
   unsigned  pgpVc;

   // Data
   unsigned   size;  // dwords

} PgpCardTx;

// RX Structure
typedef struct {
    unsigned   model; // large=8, small=4
    unsigned   maxSize; // dwords
    unsigned*  data;

   // Lane & VC
   unsigned    pgpLane;
   unsigned    pgpVc;

   // Data
   unsigned   rxSize;  // dwords

   // Error flags
   unsigned   eofe;
   unsigned   fifoErr;
   unsigned   lengthErr;

} PgpCardRx;

unsigned            debug               = 0;

enum {uDefaultMaxDataSize=1024, ucDefaultTTL=32, uDefaultPort=10148, uDefaultAddr=239<<24|255<<16|24<<8|1};

char* addressToStr( unsigned int uAddr, char* s) {
  unsigned int uNetworkAddr = htonl(uAddr);
  const unsigned char* pcAddr = (const unsigned char*) &uNetworkAddr;
  sprintf(s, "%u.%u.%u.%u", pcAddr[0], pcAddr[1], pcAddr[2], pcAddr[3]);
  return s;
}

class MyEvrReceiver {
  public:
    MyEvrReceiver(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize,
          char* sInterfaceIp = NULL);
    ~MyEvrReceiver() {};
    int fetch(unsigned int iBufSize, void* pFetchBuffer, unsigned int& iRecvDataSize);

  private:
    int               _iSocket;
    unsigned int      _uMaxDataSize;
    unsigned int      _uAddr, _uPort;
    bool              _bInitialized;

    struct msghdr      _hdr; // Control structure socket receive
    struct iovec       _iov; // Buffer description socket receive
    struct sockaddr_in _src; // Socket name source machine
};

MyEvrReceiver::MyEvrReceiver(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize, char* sInterfaceIp)  :
    _iSocket(-1), _uMaxDataSize(uMaxDataSize), _uAddr(uAddr), _uPort(uPort), _bInitialized(false) {
  _iSocket    = socket(AF_INET, SOCK_DGRAM, 0);
  if ( _iSocket == -1 ) printf("pnccdwidget MyEvrReceiver socket() failed!\n");
  else {
    int iRecvBufSize = _uMaxDataSize + sizeof(struct sockaddr_in);
    iRecvBufSize += 2048;

    if ( iRecvBufSize < 5000 ) iRecvBufSize = 5000;
    if (setsockopt(_iSocket, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBufSize, sizeof(iRecvBufSize)) == -1) {
      printf("pnccdwidget MyEvrReceiver setsockopt(...SO_RCVBUF) failed!\n");
    } else {
      int iYes = 1;
      if(setsockopt(_iSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&iYes, sizeof(iYes)) == -1) {
        printf("pnccdwidget MyEvrReceiver setsockopt(...SO_REUSEADDR) failed!\n");
      } else {
        sockaddr_in sockaddrSrc;
        sockaddrSrc.sin_family      = AF_INET;
        sockaddrSrc.sin_addr.s_addr = htonl(_uAddr);
        sockaddrSrc.sin_port        = htons(_uPort);
        if (bind( _iSocket, (struct sockaddr*) &sockaddrSrc, sizeof(sockaddrSrc) ) == -1 ) {
          printf("pnccdwidget MyEvrReceiver bind() failed!\n");
        } else {
          sockaddr_in sockaddrName;
          unsigned iLength = sizeof(sockaddrName);
          if(getsockname(_iSocket, (sockaddr*)&sockaddrName, &iLength) != 0) {
            printf("pnccdwidget MyEvrReceiver getsockname() failed!\n");
          } else {
            char s[20];
            unsigned int uSockAddr = ntohl(sockaddrName.sin_addr.s_addr);
            unsigned int uSockPort = (unsigned int )ntohs(sockaddrName.sin_port);
            printf( "pnccdwidget Server addr: %s  Port %u  Buffer Size %u\n", addressToStr(uSockAddr, s), uSockPort, iRecvBufSize );
            memset((void*)&_hdr, 0, sizeof(_hdr));
            _hdr.msg_name       = (caddr_t)&_src;
            _hdr.msg_namelen    = sizeof(_src);
            _hdr.msg_iov        = &_iov;
            _hdr.msg_iovlen     = 1;
            in_addr_t uiInterface;
            if (sInterfaceIp == NULL || sInterfaceIp[0] == 0 ) { uiInterface = INADDR_ANY; }
            else {
              if ( sInterfaceIp[0] < '0' || sInterfaceIp[0] > '9' ) {
                struct ifreq ifr;
                strcpy( ifr.ifr_name, sInterfaceIp );
                int iError = ioctl( _iSocket, SIOCGIFADDR, (char*)&ifr );
                if ( iError == 0 ) uiInterface = ntohl( *(unsigned int*) &(ifr.ifr_addr.sa_data[2]) );
                else {
                  printf( "Cannot get IP address from network interface %s\n", sInterfaceIp );
                  uiInterface = 0;
                }
              } else {
                uiInterface = ntohl(inet_addr(sInterfaceIp));
              }
            }
            if ( uiInterface != 0 ) {
              printf( "pnccdwidget multicast interface IP: %s\n", addressToStr(uiInterface, s) );
              struct ip_mreq ipMreq;
              memset((char*)&ipMreq, 0, sizeof(ipMreq));
              ipMreq.imr_multiaddr.s_addr = htonl(_uAddr);
              ipMreq.imr_interface.s_addr = htonl(uiInterface);
              if (setsockopt (_iSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipMreq, sizeof(ipMreq)) < 0) {
                printf("pnccdwidget MyEvrReceiver setsockopt(...IP_ADD_MEMBERSHIP) failed!\n");
              } else {
                _bInitialized = true;
              }
            }
          }
        }
      }
    }
  }
}

int MyEvrReceiver::fetch(unsigned int iBufSize, void* pFetchBuffer, unsigned int& iRecvDataSize) {
  if ( iBufSize > _uMaxDataSize || pFetchBuffer == NULL ) {
    printf( "pnccdwidget MyEvrReceiver::fetch() : Input parameter invalid\n" );
    return 1;
  }
  if ( !_bInitialized ) {
    printf( "pnccdwidget MyEvrReceiver::fetch() : MyEvrReceiver is not initialized successfully\n" );
    return 2;
  }
  _iov.iov_base = pFetchBuffer;
  _iov.iov_len  = iBufSize;
  int iFlags = 0;
  iRecvDataSize = recvmsg(_iSocket, &_hdr, iFlags);
  return 0;
}

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

class pnCCDQuad  {
  public:
    pnCCDQuad();
    ~pnCCDQuad() {};
    void framenumber(unsigned n) { frameNumber = n;}
    unsigned lane() { return magic & 3;}
    void lane (unsigned l) { magic &= 0xfffffffc; magic |= (l&3); }
    void fill(uint16_t);
  public:
    unsigned magic;
    unsigned frameNumber;
    unsigned timeStampHi;
    unsigned timeStampLo;
    uint16_t data[pnCCDPayloadSize/sizeof(uint16_t)];
};

pnCCDQuad::pnCCDQuad() : magic(pnCCDMagicWord), frameNumber(0), timeStampHi(0), timeStampLo(0) {}

void pnCCDQuad::fill(uint16_t v) {
  unsigned i = 0;
  uint16_t foo;
  while (i < pnCCDPayloadSize/sizeof(uint16_t)) {
    foo = v + i%v;
    swab(&foo, &data[i++], 2);
  }
}

class pnCCDFrame {
  public:
    pnCCDFrame();
    ~pnCCDFrame() {};
    void framenumber(unsigned);
  public:
    pnCCDQuad q[4];
};

pnCCDFrame::pnCCDFrame() {
  int i = 0;
  while (i<4) {
    q[i].lane(includeElementID ? i : 0);
    q[i].fill(foobar[i]);
    i += 1;
  }
}

void pnCCDFrame::framenumber(unsigned n) {
  int i = 0;
  while (i<4) {
    q[i++].framenumber(n);
  }
  return;
}

class pnCCDFrameBuffers {
  public:
    pnCCDFrameBuffers();
    ~pnCCDFrameBuffers() {};
    pnCCDFrame* next();
    pnCCDFrame* writeNext();
    pnCCDFrame* readNext();
    void        startFrame(unsigned s) { frameNumber = s; }
  public:
    unsigned frameNumber;
    unsigned frameIndex;
    pnCCDFrame buffers[numberOfBuffers];
    void pgpcard(int f) { fd = f; }
  private:
    int             fd;
    PgpCardTx       tx[4];
    PgpCardRx       rx[4];
    unsigned        flip;
};

pnCCDFrameBuffers::pnCCDFrameBuffers() : frameNumber(0), frameIndex(0) {
  unsigned i=0;
  while (i<4) {
    // tx structure initialization
    tx[i].model = sizeof(&tx);
    tx[i].cmd   = IOCTL_Normal_Write;
    tx[i].pgpLane = i;
    tx[i].pgpVc = 0;
    tx[i].size  = sizeof(pnCCDQuad)/sizeof(unsigned);
    // rx structure initialization
    rx[i].model = sizeof(&rx);
    rx[i++].maxSize = sizeof(pnCCDQuad)/sizeof(unsigned);
  }
  flip = 0;
}

pnCCDFrame* pnCCDFrameBuffers::next() {
  pnCCDFrame* ret = &buffers[frameIndex = frameIndex++ % numberOfBuffers];
  ret->framenumber(frameNumber++);
  return ret;
}

pnCCDFrame* pnCCDFrameBuffers::writeNext() {
  pnCCDFrame* f = next();
  unsigned i=0;
  for (i=0; i<4; i++) {
    tx[i].data = (unsigned*)&(f->q[i]);
  }
  if ((debug & 1) || (f->q[0].frameNumber%printModuloBase == 0)) printf("transmitting %9u ", f->q[0].frameNumber);
  for (i=0; i<4; i++) {
    if (laneMask & (1<<i)) {
      write(fd, &tx[(flip + i) % 4], sizeof(tx[i]));
      if (debug & 1 || (f->q[0].frameNumber%printModuloBase == 0)) printf("%u ", tx[(flip + i) % 4].pgpLane);
    }
  }
  if (debug & 1 || (f->q[0].frameNumber%printModuloBase == 0)) printf("\n");
  if (flipFlag) flip = (flip + 1) % 4;
  return f;
}

pnCCDFrame* pnCCDFrameBuffers::readNext() {
  pnCCDFrame* f = next();
  if (debug & 8) printf("readNext frame*(%p)\n", f);
  unsigned i=0;
  int size = 0;
  while (i<4) {
    if (laneMask & (1<<i)) {
      rx[i].data = (unsigned*)&(f->q[i]);
      if (debug & 8) printf("readNext %u readstruc(%p) data(%p) address of quad(%p)\n", i, &rx[i], rx[i].data, &(f->q[i]));
      else {
        size = read(fd, &(rx[i]), sizeof(rx[i]));
        if (size<0) perror("readNext");
        else if ((unsigned)size != rx[i].maxSize) {
          printf("readNext read wrong size %u not %u in frame %u\n", size, rx[i].maxSize, f->q[i].frameNumber);
        } else {
          f->q[i].lane(rx[i].pgpLane);
        }
      }
    }
    i++;
  }
  return f;
}

pnCCDFrameBuffers* bufs;

void sigHandler( int signal ) {
  psignal( signal, "Signal received by pnccdWidget");
  if (writing) fclose(writeFile);
  printf("Signal handler pulling the plug after %u lane errors\n", laneErrors);
  ::exit(signal);
}



//using namespace Pds;

void printUsage(char* name) {
  printf( "Usage: %s [-h]  -P <pgpcardNumb> [-w][-W count,delay][-r][-R][-o maxPrint][-s filename][-S start]"
      "[-x count,delay][-e port,multAddr][-i eth][-F <val>][-D <debug>][-p pf][-f f0,f1,f2,f3][-M <mod>][-Z]\n"
      "    -h      Show usage\n"
      "    -P      Set pgpcard index number  (REQUIRED)\n"
      "                The format of the index number is a one byte number with the bottom nybble being\n"
      "                the index of the card and the top nybble being a port mask where one bit is for\n"
      "                each port, but a value of zero maps to 15 for compatibility with unmodified\n"
      "                applications that use the whole card\n"
      "    -w      Write packet to all four lanes\n"
      "    -W      Loop writing register to destination count times, unless count = 0 which means forever\n"
      "                 delay is time between frames sent in microseconds\n"
      "    -r      Read packet from front end, resulting data treatment by other options\n"
      "    -R      Loop reading data until interrupted, resulting data treatment by other options\n"
      "    -o      Print out up to maxPrint words when reading data\n"
      "    -s      Save to file when reading data\n"
      "    -S      Seed frame number start when transmitting\n"
      "    -x      loop back write / read test count times, separated by delay microseconds\n"
      "    -e      Sync to evr multicasts coming from multAddr to port\n"
      "    -i      Interface to use if syncing to evr multicasts\n"
      "    -F      Set flip flag to value given\n"
      "    -m      Set lane mask to value give, 4 bits, one for each lane\n"
      "                N.B. the mask is for looping commands only!!!!\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          print out progress\n"
      "    -p      set print flag to value given\n"
      "    -f      Set quadrant fill values to those given\n"
      "    -M      Set print modulo base to value given\n"
      "    -Z      Remove elementID from the quads being transmitted!\n",
      name
  );
}

enum Commands{none, writeCommand,readCommand,loopReadCommand,dumpCommand,testCommand,loopWriteCommand,loopWriteReadCommand, numberOfCommands};

int main( int argc, char** argv )
{
  unsigned            pgpcard             = 0;
  unsigned            command             = none;
  unsigned            count               = 0;
  unsigned            delay               = 1000;
  unsigned            printFlag           = 0;
  unsigned            maxPrint            = 0;
  bool                cardGiven           = false;
  bool                evrActive           = false;
  unsigned            idx                 = 0;
  unsigned            i                   = 0;
  unsigned            startFrame          = 0;
  pnCCDFrame*         f                   = 0;
  PgpCardTx           p;
  timespec            then, now;
  long long int       nanoSeconds        = 0LL;
  float               Hertz;
  ::signal( SIGINT, sigHandler );
  std::vector<unsigned char> vcFetchBuffer( uDefaultMaxDataSize );
  unsigned int        uAddr              = uDefaultAddr;
  unsigned int        uPort              = uDefaultPort;
  unsigned int        uMaxDataSize       = uDefaultMaxDataSize;
  unsigned int        receivedSize;
  MyEvrReceiver*       myEvrReceiver     = 0;
  char                writeFileName[256] = {""};
  char*               endptr;
  extern char*        optarg;
  char*               sInterfaceP        = NULL;
  char*               interfaceP         = (char*) calloc(20, 1);
//  char*               uAddrStr           = (char*) calloc(20, 1);
  int c;
  while( ( c = getopt( argc, argv, "hP:wW:D:P:rRo:s:S:x:e:i:p:F:m:f:M:Z" ) ) != EOF ) {
     switch(c) {
      case 'P':
        pgpcard = strtoul(optarg, NULL, 0);
        cardGiven = true;
        break;
      case 'Z':
        includeElementID = false;
        printf("pnccdwidget will not include element IDs in outgoing packets\n");
        break;
      case 'M':
        printModuloBase = strtoul(optarg, NULL, 0);
        printf("pnccdwidget print modulo base %u\n", printModuloBase);
        break;
      case 'f':
        foobar[0] = strtoul(optarg,&endptr,0);
        foobar[1] = strtoul(endptr+1,&endptr,0);
        foobar[2] = strtoul(endptr+1,&endptr,0);
        foobar[3] = strtoul(endptr+1,&endptr,0);
        printf("pnccdwidget fill quads with ");
        for (i=0; i<4; i++) printf("%u%s", foobar[i], i<3 ? ", " : "\n");
        break;
      case 'm':
        laneMask = strtoul(optarg, NULL, 0) & 0xf;
        printf("pnccdwidget lane mask 0x%x\n", laneMask);
        break;
      case 'F':
        flipFlag = (strtoul(optarg, NULL, 0) != 0);
        printf("pnccdwidget flip flag is %s\n", flipFlag ? "true" : "false");
        break;
        case 'D':
        debug = strtoul(optarg, NULL, 0);
        printf("pnccdwidget debug value given %u\n", debug);
        break;
      case 'w':
        command = writeCommand;
        break;
      case 'W':
        command = loopWriteCommand;
        count = strtoul(optarg,&endptr,0);
        delay = strtoul(endptr+1,&endptr,0);
        break;
     case 'r':
        command = readCommand;
        break;
      case 'R':
        command = loopReadCommand;
        break;
      case 'o':
        maxPrint = strtoul(optarg, NULL, 0);
        break;
      case 's':
        strcpy(writeFileName, optarg);
        writing = true;
        break;
      case 'S':
        startFrame = strtoul(optarg, NULL, 0);
        break;
      case 'x':
        command = loopWriteReadCommand;
        count = strtoul(optarg,&endptr,0);
        delay = strtoul(endptr+1,&endptr,0);
        break;
      case 'e':
        uPort = strtoul(optarg,&endptr,0);
        uAddr = ntohl(inet_addr(endptr+1));
        evrActive = true;
        break;
      case 'i':
        sInterfaceP = interfaceP;
        strcpy(sInterfaceP, optarg);
        break;
      case 'p':
        printFlag = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        printUsage(argv[0]);
        return 0;
        break;
      default:
        printf("pnccdwidget Error: Option could not be parsed, or is not supported yet!\n");
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
  if (debug & 1) printf("pnccdwidget using %s\n", devName);
  if (fd < 0) {
    sprintf(err, "pnccdWidget opening %s failed", devName);
    perror(err);
    // What else to do if the open fails?
    return 1;
  }

  bufs = new pnCCDFrameBuffers();
  bufs->startFrame(startFrame);

  if (evrActive) {
    myEvrReceiver = new MyEvrReceiver::MyEvrReceiver(uAddr, uPort, uMaxDataSize, sInterfaceP);
  }

  if (debug & 1) printf("pnccdwidget writing frame size %u\n", pnCCDFrameSize);
  else printf("pnccdwidget debug value %u\n", debug);

  p.model = sizeof(&p);
  p.cmd   = IOCTL_Write_Scratch;
  p.data  = (unsigned*)pnCCDFrameSize;
  write(fd, &p, sizeof(p));

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

  bufs->pgpcard(fd);

  bool keepGoing = true;
  bool firstLoop = true;

  printf("pnCCD frame size %d\n", (int) (unsigned long int)sizeof(pnCCDFrame));

  switch (command) {
    case none:
      printf("pnccdWidget - no command given, exiting\n");
      return 0;
      break;
    case writeCommand:
      f = bufs->writeNext();
      if (printFlag)  printf("pnccdWidget wrote frame %u\n", f->q[0].frameNumber);
      break;
    case loopWriteCommand:
      if (count==0) count = 0xffffffff;
      idx = 0;
      if (!evrActive) {
        printf("pnccdwidget looping writing %u times separated by %u microseconds\n", count, delay);
      }
      while (count--) {
        if (evrActive) {
          myEvrReceiver->fetch(uMaxDataSize, &vcFetchBuffer[0], receivedSize);
        } else {
          usleep(delay);
        }
        f = bufs->writeNext();
        if (printFlag || firstLoop)  {
          firstLoop = false;
          if (debug & 1) printf("pnccdWidget wrote frame %u\n", f->q[0].frameNumber);
        }
      }
      break;
    case readCommand:
      if (debug & 1) printf("pnccdwidget reading next\n");
      f = bufs->readNext();
      {
        unsigned mask = 0;
        for (idx=1; idx<4; idx++) if (f->q[0].frameNumber != f->q[idx].frameNumber) mask = 1 << idx;
        if (mask) {
          printf("read out of sync 0x%x ", mask);
          for (idx=0; idx<4; idx++) printf("%u ", f->q[idx].frameNumber);
          printf("\n");
        } else if (printFlag) {
          printf("read frame %u\n", f->q[0].frameNumber);
        }
        if (writing) {
          for (idx=0; idx<4; idx++) {
            i = 0;
            while (idx != f->q[i].lane() && i<4) {i++;}
            if (i!=4) fwrite(&f->q[i], sizeof(pnCCDQuad), 1, writeFile);
            else {
              printf("loopRead writing failure to find lanes which are: ");
              for (i=0; i<4; i++) printf("%u ", f->q[i].lane());
              printf("\n");
            }
          }
        }
      }
      break;
    case loopReadCommand:
      while (keepGoing) {
        if (debug & 2) printf("pnccdwidget reading next\n");
        f = bufs->readNext();
        unsigned mask = 0;
        for (idx=1; idx<4; idx++) if (f->q[0].frameNumber != f->q[idx].frameNumber)     {
          if (laneMask == 0xf) {
            mask = 1 << idx;
          }
        }
        if (mask) {
          printf("read out of sync %u ", idx);
          for (idx=0; idx<4; idx++) printf("%u ", f->q[idx].frameNumber);
          printf("\n");
//          keepGoing = false;
        } else if ( (debug & 1) | (f->q[0].frameNumber % 1000 == 0)) {
          if (f->q[0].frameNumber) {
            clock_gettime(CLOCK_REALTIME, &now);
            nanoSeconds = timeDiff(&now, &then);
            Hertz = 1000.0 / (((float)nanoSeconds) / 1e9);
            then = now;
          } else {
            clock_gettime(CLOCK_REALTIME, &then);
            Hertz = 0.0;
          }
          if (debug & 2) printf("pnccdwidget read frame %u %6.2fHz\n", f->q[0].frameNumber, Hertz);
        }
        if (maxPrint) {
          for (idx=0; idx<4; idx++) {
            uint16_t* u = (uint16_t*)&(f->q[idx]);
            printf("pnccd frame %u quad %u  ", f->q[idx].frameNumber, f->q[idx].lane());
            for (i=0; i<maxPrint; i++) {
              printf("0x%x ", u[i]);
            }
            printf("\n");
          }
          printf("\n");
        }
        if (writing) {
          for (idx=0; idx<4; idx++) {
            if (laneMask & (1<<idx)) {
              i = 0;
              while (idx != f->q[i].lane() && i<4) {i++;}
              if (i!=4) fwrite(&f->q[i], sizeof(pnCCDQuad), 1, writeFile);
              else {
                printf("loopRead writing failure to find lanes which are: ");
                for (i=0; i<4; i++) printf("%u ", f->q[i].lane());
                printf("\n");
                keepGoing = false;
              }
            }
          }
        }
      }
      break;
    case loopWriteReadCommand:
      if (laneMask == 0xf) {
        if (evrActive) {
          //        printf("pnccdwidget looping writing %u times triggered by evr multicasts %s on %u from %s\n",
          //            count, addressToStr(uAddr, uAddrStr), uPort, interfaceP);
        } else {
          printf("pnccdwidget looping writing %u times separated by %u microseconds\n", count, delay);
        }
        while (count--) {
          if (evrActive) {
            myEvrReceiver->fetch(uMaxDataSize, &vcFetchBuffer[0], receivedSize);
          } else {
            usleep(delay);
          }
          f = bufs->writeNext();
          if (printFlag)  printf("pnccdWidget wrote frame %u\n", f->q[0].frameNumber);
          f = bufs->readNext();
          {
            unsigned mask = 0;
            for (idx=1; idx<4; idx++) if (f->q[0].frameNumber != f->q[idx].frameNumber) mask = 1 << idx;
            if (mask) {
              printf("read out of sync %u ", idx);
              for (idx=0; idx<4; idx++) printf("%u ", f->q[idx].frameNumber);
              printf("\n");
            } else if (printFlag | (f->q[0].frameNumber % 1000 == 0)) {
              if (f->q[0].frameNumber) {
                clock_gettime(CLOCK_REALTIME, &now);
                nanoSeconds = timeDiff(&now, &then);
                Hertz = 1000.0 / (((float)nanoSeconds) / 1e9);
                then = now;
              } else {
                clock_gettime(CLOCK_REALTIME, &then);
                Hertz = 0.0;
              }
              printf("pnccdwidget read frame %u %6.2fHz\n", f->q[0].frameNumber, Hertz);
            }
          }
        }
      }
      break;
    default:
      printf("pnccdWidget - unknown command, exiting\n");
      break;
  }
  return 0;
}
