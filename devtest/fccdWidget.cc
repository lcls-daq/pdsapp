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
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include </usr/include/netinet/in.h>
#include <endian.h>
FILE*               binFile           = 0;
unsigned            printModuloBase     = 1000;
uint16_t            frameCount          = 0;
uint8_t             packetCount         = 0;
unsigned            frameSize           = 0;
unsigned            extraPixels         = 3664;
unsigned            IPAddrToSendFrom    = 0xc0a80366;
unsigned            IPAddrToSendTo      = 0xc0a80365;
unsigned            IPPortToSendTo      = 30060;
uint8_t  sof[5] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4};
uint8_t  eof[8] = {0xF0, 0xF1, 0xF2, 0xDE, 0xAD, 0xF0, 0x0D, 0x01};
enum {fccdFrameSize=1843208, bytesPerPacket=8184, udpJumbo=9000};
enum { SendFlags=0};

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
  if ( _iSocket == -1 ) printf("fccdwidget MyEvrReceiver socket() failed!\n");
  else {
    int iRecvBufSize = _uMaxDataSize + sizeof(struct sockaddr_in);
    iRecvBufSize += 2048;

    if ( iRecvBufSize < 5000 ) iRecvBufSize = 5000;
    if (setsockopt(_iSocket, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBufSize, sizeof(iRecvBufSize)) == -1) {
      printf("fccdwidget MyEvrReceiver setsockopt(...SO_RCVBUF) failed!\n");
    } else {
      int iYes = 1;
      if(setsockopt(_iSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&iYes, sizeof(iYes)) == -1) {
        printf("fccdwidget MyEvrReceiver setsockopt(...SO_REUSEADDR) failed!\n");
      } else {
        sockaddr_in sockaddrSrc;
        sockaddrSrc.sin_family      = AF_INET;
        sockaddrSrc.sin_addr.s_addr = htonl(_uAddr);
        sockaddrSrc.sin_port        = htons(_uPort);
        if (bind( _iSocket, (struct sockaddr*) &sockaddrSrc, sizeof(sockaddrSrc) ) == -1 ) {
          printf("fccdwidget MyEvrReceiver bind() failed!\n");
        } else {
          sockaddr_in sockaddrName;
          unsigned iLength = sizeof(sockaddrName);
          if(getsockname(_iSocket, (sockaddr*)&sockaddrName, &iLength) != 0) {
            printf("fccdwidget MyEvrReceiver getsockname() failed!\n");
          } else {
            char s[20];
            unsigned int uSockAddr = ntohl(sockaddrName.sin_addr.s_addr);
            unsigned int uSockPort = (unsigned int )ntohs(sockaddrName.sin_port);
            printf( "fccdwidget Server addr: %s  Port %u  Buffer Size %u\n", addressToStr(uSockAddr, s), uSockPort, iRecvBufSize );
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
              printf( "fccdwidget multicast interface IP: %s\n", addressToStr(uiInterface, s) );
              struct ip_mreq ipMreq;
              memset((char*)&ipMreq, 0, sizeof(ipMreq));
              ipMreq.imr_multiaddr.s_addr = htonl(_uAddr);
              ipMreq.imr_interface.s_addr = htonl(uiInterface);
              if (setsockopt (_iSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipMreq, sizeof(ipMreq)) < 0) {
                printf("fccdwidget MyEvrReceiver setsockopt(...IP_ADD_MEMBERSHIP) failed!\n");
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
    printf( "fccdwidget MyEvrReceiver::fetch() : Input parameter invalid\n" );
    return 1;
  }
  if ( !_bInitialized ) {
    printf( "fccdwidget MyEvrReceiver::fetch() : MyEvrReceiver is not initialized successfully\n" );
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

void printSock(sockaddr_in* a) {
  char* cp = (char*) a;
  for (unsigned i=0; i<sizeof(sockaddr_in); i++) {
    printf("%2hhx ", cp[i]);
  }
  printf("\n");
}

void printHdr(msghdr* a) {
  char* cp = (char*) a;
  for (unsigned i=0; i<sizeof(msghdr); i++) {
    printf("%2hhx ", cp[i]);
  }
  printf("\n");
}

class fccdPacketHeader {
  public:
    fccdPacketHeader();
    ~fccdPacketHeader() {};

    uint16_t frameCount() { return _frameCount;}
    void packetCount(uint8_t p) {_packetCount = p;}
    void frameCount(uint16_t f) {_frameCount =f;}
    void incrementPacketCount() {_packetCount += 1;}
    void incrementFrameCount()  {_frameCount += 1;}

    uint8_t     _packetCount;
    uint8_t     _startFlag[5];
    uint16_t    _frameCount;
};

fccdPacketHeader::fccdPacketHeader() : _packetCount(0), _frameCount(0) {
  memcpy(_startFlag, sof, 5);
}

class fccdFrame {
  public:
    fccdFrame(char*);
    ~fccdFrame() {};
    void framenumber(unsigned);
    int write();
    fccdPacketHeader* header() { return &_phdr; }
  public:
    uint8_t          _packetCount;
    uint16_t         _frameCount;
    fccdPacketHeader _phdr;
    int             _sock;
    int             _parm;
    sockaddr_in     _socketAddr;
    sockaddr_in     _mySocketAddr;
    in_addr         _myAddress;
    struct msghdr   hdr;
    struct iovec    iov[2];
    unsigned       fullChunksPerFrame;
    unsigned       sizeOfLastPartial;
    uint8_t*         _frame;
};

fccdFrame::fccdFrame(char* fileName) : _packetCount(0), _frameCount(0), _parm(udpJumbo*1000) {
  size_t ret;
  frameSize = fccdFrameSize + (extraPixels * sizeof(uint16_t));
  fullChunksPerFrame = frameSize / bytesPerPacket;
  sizeOfLastPartial  = frameSize % bytesPerPacket;
  _frame = (uint8_t*) calloc(frameSize + 1<<17, sizeof(uint8_t));
  printf("Opening %s to read bin file from\n", fileName);
  FILE* binFile = fopen(fileName, "r");
  if (!binFile) {
    char s[200];
    sprintf(s, "Could not open %s ", fileName);
    perror(s);
    exit(-1);
  }
  ret = fread(_frame, sizeof(uint8_t), frameSize, binFile);
  if (ret != frameSize) {
    if (feof(binFile)) {
      printf("fccdFrame constructor: bin file was too short, was %u not %u\n", ret, frameSize);
    } else if (ferror(binFile)) {
      perror("fccdFrame constructor bin file read");
    }
    exit(-1);
  }
  fclose(binFile);
  _myAddress.s_addr  = htonl(0xc0a80366); //IPAddrToSendFrom);
  hdr.msg_iov   = iov;
  hdr.msg_iovlen       = 2;
  hdr.msg_name         = (caddr_t)&_socketAddr;
  hdr.msg_namelen      = sizeof(sockaddr_in);
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;
  if ((_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("transmitter socket() failed");
    pthread_exit(NULL);
  }
  if(setsockopt(_sock, SOL_SOCKET, SO_SNDBUF, (char*)&_parm, sizeof(_parm)) < 0) {
    perror("setsockopt(SO_SNDBUF) failed transmitter thread");
    pthread_exit(NULL);
  }
//  if (setsockopt(_sock, IPPROTO_IP, IP_MULTICAST_IF, &_myAddress.s_addr, sizeof(unsigned)) < 0) {
//    char str[120];
//    sprintf(str, "setsockopt(IP_MULTICAST_IF) failed transmitter thread address 0x%x ", _myAddress.s_addr);
//    perror(str);
//    pthread_exit(NULL);

  memset(&_mySocketAddr, 0, sizeof(_mySocketAddr));       // Zero out structure
  _mySocketAddr.sin_family = AF_INET;                     // Internet address family
  _mySocketAddr.sin_addr.s_addr = htonl(IPAddrToSendFrom);// Incoming interface
  _mySocketAddr.sin_port = 0;         //  port

  memset(&_socketAddr, 0, sizeof(_socketAddr));         // Zero out structure
  _socketAddr.sin_family = AF_INET;                     // Internet address family
  _socketAddr.sin_addr.s_addr = htonl(IPAddrToSendTo);  // Incoming interface
  _socketAddr.sin_port = htons(IPPortToSendTo);         //  port

  if (bind(_sock, (struct sockaddr *) &_mySocketAddr, sizeof(_mySocketAddr)) < 0) {
    perror("bind() failed transmitter thread");
    exit(-1);
  }
}

int fccdFrame::write() {
  unsigned bytes0 = 0;
  unsigned bytes1 = 0;
  unsigned packets = 0;
  unsigned i;
  iov[0].iov_base = (caddr_t)&_phdr;
  iov[0].iov_len = sizeof(fccdPacketHeader);
  iov[1].iov_len = bytesPerPacket;
  for (i=0; i<fullChunksPerFrame; i++) {
    iov[1].iov_base = _frame + (i * bytesPerPacket);
    if (sendmsg(_sock, &hdr, SendFlags) < 0) {
      perror("Transmitting full sendmsg _sock");
      printf("On chunk %u\n", i);
      printf("_socketAddr:   "); printSock(&_socketAddr);
      printf("hdr: "); printHdr(&hdr);
      return -1;
    }
    packets += 1;
    bytes0 += iov[0].iov_len;
    bytes1 += iov[1].iov_len;
    _phdr.incrementPacketCount();
  }
  iov[1].iov_len = sizeOfLastPartial;
  iov[1].iov_base = (caddr_t)(_frame + (fullChunksPerFrame * bytesPerPacket));
//  printf("writing %u, %u bytes from %p\n", i, iov[0].iov_len + iov[1].iov_len, iov[1].iov_base);
  if (sendmsg(_sock, &hdr, SendFlags) < 0) {
    perror("Transmitting last sendmsg _sock");
    return -1;
  }
  packets += 1;
  bytes0 += iov[0].iov_len;
  bytes1 += iov[1].iov_len;
  if (!(_phdr.frameCount()%1000)) printf("wrote %u packets and %u header bytes and %u payload bytes for frame %u\n",
      packets, bytes0, bytes1, _phdr.frameCount());
  _phdr.incrementFrameCount();
  _phdr.packetCount(0);
  return 0;
}


void sigHandler( int signal ) {
  psignal( signal, "Signal received by pnccdWidget");
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}



//using namespace Pds;

void printUsage(char* name) {
  printf( "Usage: %s [-h]  [-w][-W count,delay][-b filename]"
      "[-e port,multAddr][-i eth][-f fromAddr][-t toAddr][-p toPort][-D debug][-P pf]\n"
      "    -h      Show usage\n"
      "    -w      Write one frame\n"
      "    -W      Loop writing count frames, unless count = 0 which means forever\n"
      "                 delay is time between frames sent in microseconds\n"
      "    -b      Binary image file name to read from\n"
      "    -f      From address to send frames\n"
      "    -t      To address to send frames\n"
      "    -e      Sync to evr multicasts coming from multAddr to port\n"
      "    -i      Interface to use if syncing to evr multicasts\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          print out progress\n"
      "    -P      set print flag to value given\n",
      name
  );
}

enum Commands{none, writeCommand,loopWriteCommand, numberOfCommands};

int main( int argc, char** argv )
{
  unsigned            command             = none;
  unsigned            count               = 0;
  unsigned            delay               = 1000;
  unsigned            printFlag           = 0;
  bool                evrActive           = false;
  fccdFrame*          f                   = 0;
  ::signal( SIGINT, sigHandler );
  std::vector<unsigned char> vcFetchBuffer( uDefaultMaxDataSize );
  unsigned int        uAddr              = uDefaultAddr;
  unsigned int        uPort              = uDefaultPort;
  unsigned int        uMaxDataSize       = uDefaultMaxDataSize;
  unsigned int        receivedSize;
  MyEvrReceiver*       myEvrReceiver     = 0;
  char                binFileName[512] = {"NoFileNameEntered"};
  char*               endptr;
  extern char*        optarg;
  char*               sInterfaceP        = NULL;
  char*               interfaceP         = (char*) calloc(20, 1);
//  char*               uAddrStr           = (char*) calloc(20, 1);
  int c;
  while( ( c = getopt( argc, argv, "hwW:D:b:f:t:p:e:i:P:" ) ) != EOF ) {
     switch(c) {
      case 'w':
        command = writeCommand;
        break;
      case 'W':
        command = loopWriteCommand;
        count = strtoul(optarg,&endptr,0);
        delay = strtoul(endptr+1,&endptr,0);
        break;
      case 'b':
        strcpy(binFileName, optarg);
        break;
      case 'f':
        IPAddrToSendFrom = strtoul(optarg, NULL, 0);
        break;
      case 't':
        IPAddrToSendTo = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        IPPortToSendTo = strtoul(optarg, NULL, 0);
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
      case 'D':
        debug = strtoul(optarg, NULL, 0);
        printf("fccdwidget debug value given %u\n", debug);
        break;
      case 'P':
        printFlag = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        printUsage(argv[0]);
        return 0;
        break;
      default:
        printf("fccdwidget Error: Option could not be parsed, or is not supported yet!\n");
        printUsage(argv[0]);
        return 0;
        break;
    }
  }


  f = new fccdFrame(binFileName);
  printf("fccdwidget sending from address 0x%x", IPAddrToSendFrom);
  printf(" and to address 0x%x", IPAddrToSendTo);
  printf(" and port %u\n", IPPortToSendTo);

  if (evrActive) {
    myEvrReceiver = new MyEvrReceiver(uAddr, uPort, uMaxDataSize, sInterfaceP);
  }

  bool firstLoop = true;

  printf("fCCD frame size %u\n", frameSize);

  switch (command) {
    case none:
      printf("pnccdWidget - no command given, exiting\n");
      return 0;
      break;
    case writeCommand:
      f->write();
      if (printFlag)  printf("pnccdWidget wrote frame %u\n", f->_phdr.frameCount());
      break;
    case loopWriteCommand:
      if (count==0) count = 0xffffffff;
       if (!evrActive) {
        printf("fccdwidget looping writing %u times separated by %u microseconds\n", count, delay);
      }
      while (count--) {
        if (evrActive) {
          myEvrReceiver->fetch(uMaxDataSize, &vcFetchBuffer[0], receivedSize);
        } else {
          usleep(delay);
        }
        f->write();
        if (printFlag || firstLoop)  {
          firstLoop = false;
          if (debug & 1) printf("fccdWidget wrote frame %u\n", f->_phdr.frameCount());
        }
      }
      printf("fccdWidget frame count %u\n", f->_phdr.frameCount());
      break;
    default:
      printf("pnccdWidget - unknown command, exiting\n");
      break;
  }
  return 0;
}
