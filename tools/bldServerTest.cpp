#include <stdio.h>
#include <memory.h>
#include <pthread.h>
#include <errno.h>
#include <getopt.h>
#include <string>
#include <vector>
#include <sstream>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "pds/service/NetServer.hh"
#include "pds/service/Ins.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "multicastConfig.hh"
#include "bldServerTest.h"

using std::string;

namespace Pds 
{

  enum DataDump_t { TY_DUMP_NONE, TY_DUMP_INT, TY_DUMP_UINT, TY_DUMP_DOUBLE };

  static std::string addressToStr( unsigned int uAddr );

  BldServerSlim::BldServerSlim(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize,
      char* sInterfaceIp) : _iSocket(-1), _uMaxDataSize(uMaxDataSize), _uAddr(uAddr), _uPort(uPort), _bInitialized(false),
      _lastEvr(0), _lastDgram(0), _fiducialGap(0)
  {
    try
    {
      /*
       * socket
       */
      _iSocket    = socket(AF_INET, SOCK_DGRAM, 0);
      if ( _iSocket == -1 ) throw string("socket() == -1");

      /*
       * set server socket options
       */
      int iRecvBufSize = _uMaxDataSize + sizeof(struct sockaddr_in);
#ifdef __linux__
      iRecvBufSize += 2048; // 1125
#endif

      if ( iRecvBufSize < 5000 )
        iRecvBufSize = 5000;
      /*
       * In NEH machine, atca201 - atca212, we need to set iRecvBufSize >= 4729
       * The actual buffer size (queried by getsockopt() ) is 9458
       * Smaller value will cause multicast receiving fails: tcpdump can see and print out the packet, but our
       * program cannot receive the packet. netstat -s will show errors in receiving packets
       */

      if(
          setsockopt(_iSocket, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBufSize, sizeof(iRecvBufSize))
          == -1)
        throw string("BldServerSlim::BldServerSlim() : setsockopt(...SO_RCVBUF) failed");

      //int iQueriedRecvBufSize, iLenQueriedRecvBufSize;
      //if(
      //  getsockopt(_iSocket, SOL_SOCKET, SO_RCVBUF, (char*)&iQueriedRecvBufSize, (socklen_t*) &iLenQueriedRecvBufSize)
      //  == -1)
      //    throw string("BldServerSlim::BldServerSlim() : getsockopt(...SO_RCVBUF) failed");
      //printf( "Org Buffer Size = %d, Queried Buffer Size = %d\n", iRecvBufSize, iQueriedRecvBufSize);

      int iYes = 1;

#ifdef VXWORKS 
      if(setsockopt(_iSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&iYes, sizeof(iYes)) == -1)
#else
        if(setsockopt(_iSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&iYes, sizeof(iYes)) == -1)
#endif
          throw string("BldServerSlim::BldServerSlim() : setsockopt(...SO_REUSEADDR) failed");

      /*
       * bind
       */
      sockaddr_in sockaddrSrc;
      sockaddrSrc.sin_family      = AF_INET;
      sockaddrSrc.sin_addr.s_addr = htonl(_uAddr);
      sockaddrSrc.sin_port        = htons(_uPort);

      if (
          bind( _iSocket, (struct sockaddr*) &sockaddrSrc, sizeof(sockaddrSrc) )
          == -1 )
        throw string("BldServerSlim::BldServerSlim() : bind() failed");

      /*
       * getsockname
       */
      sockaddr_in sockaddrName;
#ifdef __linux__
      unsigned iLength = sizeof(sockaddrName);
#else
      int iLength = sizeof(sockaddrName);
#endif      

      if(getsockname(_iSocket, (sockaddr*)&sockaddrName, &iLength) == 0)
      {
        unsigned int uSockAddr = ntohl(sockaddrName.sin_addr.s_addr);
        unsigned int uSockPort = (unsigned int )ntohs(sockaddrName.sin_port);
        printf( "Server addr: %s  Port %u  Buffer Size %u\n", addressToStr(uSockAddr).c_str(), uSockPort, iRecvBufSize );
      }
      else
        throw string("BldServerSlim::BldServerSlim() : getsockname() failed");

      /*
       * setup data header
       */
      memset((void*)&_hdr, 0, sizeof(_hdr));
      _hdr.msg_name       = (caddr_t)&_src;
      _hdr.msg_namelen    = sizeof(_src);
      _hdr.msg_iov        = &_iov;
      _hdr.msg_iovlen     = 1;

      /*
       * register multicast address
       */
      in_addr_t uiInterface;

      if (sInterfaceIp == NULL || sInterfaceIp[0] == 0 )
      {
        uiInterface = INADDR_ANY;
      }
      else
      {
        if ( sInterfaceIp[0] < '0' || sInterfaceIp[0] > '9' )
        {        
          struct ifreq ifr;
          strcpy( ifr.ifr_name, sInterfaceIp );
          int iError = ioctl( _iSocket, SIOCGIFADDR, (char*)&ifr );
          if ( iError == 0 )
            uiInterface = ntohl( *(unsigned int*) &(ifr.ifr_addr.sa_data[2]) );
          else
          {
            printf( "Cannot get IP address from network interface %s\n", sInterfaceIp );
            uiInterface = 0;
          }
        }
        else
        {
          uiInterface = ntohl(inet_addr(sInterfaceIp));
        }              
      }

      if ( uiInterface != 0 )
      {
        printf( "multicast interface IP: %s\n", addressToStr(uiInterface).c_str() );

        struct ip_mreq ipMreq;
        memset((char*)&ipMreq, 0, sizeof(ipMreq));
        ipMreq.imr_multiaddr.s_addr = htonl(_uAddr);
        ipMreq.imr_interface.s_addr = htonl(uiInterface);
        if (
            setsockopt (_iSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipMreq,
                sizeof(ipMreq)) < 0 )
          throw string("BldServerSlim::BldServerSlim() : setsockopt(...IP_ADD_MEMBERSHIP) failed");
      }

      _bInitialized = true;
    }
    catch (string& sError)
    {
      printf( "*** Error: %s, errno = %d (%s)\n", sError.c_str(), errno,
          strerror(errno) );
    }

  }

  BldServerSlim::~BldServerSlim()
  {
  }

  void BldServerSlim::lastEVR(unsigned e)
  {
    if (_lastEvr) {
      int gap;
      if ((gap = e - _lastEvr) != 1) {
        printf("\tGAP GAP GAP, evr %s %d\n", gap < 0 ? "restart" : "gap", gap - 1);
      }
    }
    _lastEvr = e;
  }

  void BldServerSlim::thisDgram(Pds::EvrDatagram* dgram) {
    if (_lastDgram) {
      if (_fiducialGap != 0) {
        unsigned gap = dgram->seq.stamp().fiducials() - _lastDgram->seq.stamp().fiducials();
        if (_fiducialGap != gap && !(gap & 0x80000000)) {
          printf("fiducials gap is %u but should be %u, new 0x%x, last 0x%x\n",
              gap, _fiducialGap, dgram->seq.stamp().fiducials(), _lastDgram->seq.stamp().fiducials());
        }
      } else {
        _fiducialGap = dgram->seq.stamp().fiducials() - _lastDgram->seq.stamp().fiducials();
        printf("Expected fiducial gap now %u\n", _fiducialGap);
      }
    } else {
      _lastDgram = &_myDgram;
    }
    _myDgram = *dgram;
  }

  int BldServerSlim::fetch(unsigned int iBufSize, void* pFetchBuffer, unsigned int& iRecvDataSize)
  {
    if ( iBufSize > _uMaxDataSize || pFetchBuffer == NULL )
    {
      printf( "BldServerSlim::fetch() : Input parameter invalid\n" );
      return 1;
    }

    if ( !_bInitialized )
    {
      printf( "BldServerSlim::fetch() : BldServerSlim is not initialized successfully\n" );
      return 2;
    }

    _iov.iov_base = pFetchBuffer;
    _iov.iov_len  = iBufSize;               

    int iFlags = 0;
    iRecvDataSize = recvmsg(_iSocket, &_hdr, iFlags);

    return 0;
  }

  string addressToStr( unsigned int uAddr )
  {
    unsigned int uNetworkAddr = htonl(uAddr);
    const unsigned char* pcAddr = (const unsigned char*) &uNetworkAddr;
    std::stringstream sstream;
    sstream << 
        (int) pcAddr[0] << "." <<
        (int) pcAddr[1] << "." <<
        (int) pcAddr[2] << "." <<
        (int) pcAddr[3];

    return sstream.str();
  }

} // namespace Pds

/*
 * Test function
 */

using namespace Pds::ConfigurationMulticast;
void printData(	const std::vector<unsigned char>& vcBuffer, int iDataSize,
				char * sTyDump, size_t cntDump, size_t offDump, bool showHdr, bool quiet );
void printSummary();

static void showUsage()
{
  printf( "Usage:  bldServerTest  [-v|--version] [-h|--help] [-a|--address <Ip Address>] [-p|--port <Port>] [-i|--interface <Interface Name/IP>]  [<Interface name/IP>]\n"
      " Options:\n"
      "   -v|--version                        Show file version\n"
      "   -h|--help                           Show Usage\n"
      "   -H|--showHdr                        Show BLD Packet Header\n"
      "   -a|--address   <Ip Address>         Set the multicast address to be listened to. Default: %s\n"
      "   -p|--port      <Port>               Set the port to be listened to. Default: %u\n"
      "   -P|--PGPtest                        PGP card test with occasional summary of received packets\n"
      "   -s|--size      <Buffer Size>        Set the max data size for allocating buffer. Default: %u\n"
      "   -e|--evrGapHunt                     Watch for and print out message if a gap is seen in EVR packets\n"
      "   -c|--cntDump                        Number of data values to dump from start of packet.  Default 0\n"
      "   -o|--offDump                        Dump offset, number of bytes of data to skip before dump.  Default 0, must be mult of 4\n"
      "   -t|--tyDump                         Type of data to dump: int, uint, double\n"
      "   -E|--evrFiducialGapCheck            Watch for anomalous fiducials period in evr multicasts\n"
      "   -q|--quiet           				  Only dump data if gap changes\n"
      "   -i|--interface <Interface Name/IP>  Set the network interface for receiving multicast. Use ether IP address (xxx.xx.xx.xx) or name (eth0, eth1,...)\n"
      "   <Interface IP/Name>                 Same as above. *This is an argument without the option (-i) flag\n",
      Pds::addressToStr(uDefaultAddr).c_str(), uDefaultPort, uDefaultMaxDataSize
  );
}

static const char sBldServerTestVersion[] = "0.94, with fiducials inconsistency check";

static void showVersion()
{
  printf( "\nVersion:  bldServerTest  Ver %s\n", sBldServerTestVersion );
}

// block of variables used only for PGP card test
enum {MagicNumber=0, QuadNumber=1, FrameCount=2, MAGIC=0xFEEDFACE, PacketsPerSegment=141};
unsigned lastFrameNumber       = 0;
unsigned lostPackets           = 0;
unsigned lostSegments          = 0;
unsigned lostEvents            = 0;
long long unsigned packetCount = 0LL;
long long unsigned dataTotal   = 0LL;
bool     packetError           = false;

void sigHandler( int signal )
{
  psignal( signal, "Signal received: ");
  printSummary();
  exit(0);
}

void * watchDogTask( void * )
{
	long long unsigned	priorPacketCount	= packetCount - 1;
	long unsigned int	nSecSincePacket		= 0;
	time_t				timeLastPacket		= 0;

	printf( "WatchDog: Monitoring packetCount ...\n" );
	while( 1 )
	{
		sleep( 1 );

		if ( priorPacketCount != packetCount )
		{
			nSecSincePacket		= 0;
			priorPacketCount	= packetCount;
			time( &timeLastPacket );
		}
		else
		{
			char	tmbuf[80];

			nSecSincePacket++;
			switch ( nSecSincePacket )
			{
			default:
				break;
			case	10:
			case	20:
			case	30:
			case	40:
			case	50:
				printf( "WatchDog: No packets in last %lu seconds!\n", nSecSincePacket );
				break;
			case	60:
				ctime_r( &timeLastPacket, tmbuf );
				printf( "WatchDog: No packets since %s\n", tmbuf );
				break;
			}
		}
	}
	return NULL;
}

int main(int argc, char** argv)
{
  bool evrGapHunt = false;
  bool PGPcardTestSummary = false;
  bool evrFiducialGapCheck = false;
  bool showHdr	= false;
  bool quiet	= false;
  int iOptionIndex = 0;
  struct option loOptions[] =
  {
      {"ver",      0, 0, 'v'},
      {"help",     0, 0, 'h'},
      {"address",  1, 0, 'a'},
      {"port",     1, 0, 'p'},
      {"PGPtest",  0, 0, 'P'},
      {"size",     1, 0, 's'},
      {"interface",1, 0, 'i'},
      {"tyDump",   1, 0, 't'},
      {"cntDump",  1, 0, 'c'},
      {"offDump",  1, 0, 'o'},
      {"evrGapHunt", 0, 0, 'e'},
      {"evrFiducialGapCheck", 0, 0, 'E'},
      {"showHdr",  0, 0, 'H'},
      {"quiet",    0, 0, 'q'},
      {0,          0, 0,  0 }
  };

  char*         sInterfaceIp  = NULL;
  unsigned int  uAddr         = uDefaultAddr;
  unsigned int  uPort         = uDefaultPort;
  unsigned int  uMaxDataSize  = uDefaultMaxDataSize;
  char		*	sTyDump		  = NULL;
  size_t		cntDump		  = 0;
  size_t		offDump		  = 0;

  while ( int opt = getopt_long(argc, argv, ":vhHa:p:Pi:s:t:c:eEq", loOptions, &iOptionIndex ) )
  {
    if ( opt == -1 ) break;

    switch(opt)
    {
      case 'v':               /* Print usage */
        showVersion();
        break;
      case 'a':
        uAddr = ntohl(inet_addr(optarg));
        break;
      case 'p':
        uPort = strtoul(optarg, NULL, 0);
        break;
      case 'P':
        PGPcardTestSummary = true;
        break;
      case 'H':
        showHdr = true;
        break;
      case 'q':
        quiet = true;
        break;
      case 's':
        uMaxDataSize = strtoul(optarg, NULL, 0);
        break;
      case 'i':
        sInterfaceIp = optarg;
        break;
      case 't':
        sTyDump = optarg;
        break;
      case 'c':
        cntDump = strtoul(optarg, NULL, 0);
        break;
      case 'o':
        offDump = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        evrGapHunt = !evrGapHunt;
        printf("Evr gap hunt now %s\n", evrGapHunt ? "ON" : "OFF");
        break;
      case 'E':
        evrFiducialGapCheck = !evrFiducialGapCheck;
        printf("Evr Fiducials anomaly hunt is now %s", evrFiducialGapCheck ? "enabled" : "disabled");
        break;
      case '?':               /* Terse output mode */
        printf( "bldServerTest:main(): Unknown option: %c\n", optopt );
        break;
      case ':':               /* Terse output mode */
        printf( "bldServerTest:main(): Missing argument for %c\n", optopt );
        break;
      default:
      case 'h':               /* Print usage */
        showUsage();
        return 0;

    }
  }


  argc -= optind;
  argv += optind;

  signal( SIGINT, sigHandler );
  if (argc >= 1 )
    sInterfaceIp = argv[0];

  Pds::BldServerSlim bldServer(uAddr, uPort, uMaxDataSize, sInterfaceIp );
  if ( !bldServer.IsInitialized() )
  {
    printf( "bldServerTest:main(): bldServerSlim is not initialzed successfully\n" );
    return 1;
  }

  std::vector<unsigned char> vcFetchBuffer( uMaxDataSize );
  Pds::EvrDatagram* dgram = (Pds::EvrDatagram*)&vcFetchBuffer[0];

  uint32_t* wp = (uint32_t*)dgram;
  unsigned pcount       = 0;
  unsigned segmentMask  = 0;

  // Launch a watchdog to let us know when packets were last received
  pthread_t		watchDogThread;
  if ( pthread_create( &watchDogThread, NULL, watchDogTask, 0 ) != 0 )
  	printf( "Error: Unable to launch watchdog task!\n" );

  printf( "Beginning Multicast Server Testing. Press Ctrl-C to Exit...\n" );

  while (1) // Pds::ConsoleIO::kbhit(NULL) == 0 )
  {
    unsigned int iRecvDataSize = 0;
    int iFail = bldServer.fetch(uMaxDataSize, &vcFetchBuffer[0], iRecvDataSize);
    if ( iFail != 0 )
    {
      printf( "bldServerTest:main(): bldServer.fetch() failed. Error Code = %d\n", iFail );
      return 2;
    }

    dataTotal += iRecvDataSize;
    ++packetCount;
    ++pcount;
    packetError = false;

    if (evrGapHunt)
    {
      bldServer.lastEVR(dgram->evr);
    }
	else {
      if (evrFiducialGapCheck) {
        bldServer.thisDgram(dgram);
      }
	  else {
        if (PGPcardTestSummary) {
          if (wp[MagicNumber] == MAGIC) {
            if (pcount != PacketsPerSegment) {
              if (pcount < PacketsPerSegment) {
                if (pcount > 1) {
                  lostPackets += PacketsPerSegment - pcount;
                  printf("lost packets %u\n", PacketsPerSegment - pcount);
                  packetError = true;
                }
              } else {
                if (unsigned remainder = pcount % PacketsPerSegment) {
                  lostPackets += PacketsPerSegment - remainder;
                  printf("lost packets %u\n", PacketsPerSegment - remainder);
                  packetError = true;
                }
              }
            }
            pcount = 0;
            if (wp[FrameCount] != lastFrameNumber) {
              if (wp[FrameCount] < lastFrameNumber) {
                // starting over
                packetCount = 0LL;
                pcount = 0;
                lastFrameNumber = 0;
                lostPackets = 0;
                lostSegments = 0;
                printf("restarting\n");
                packetError = true;
              } else {
                if (segmentMask != 0xf) {
                  unsigned thistime = 0;
                  for (unsigned i = 1; i< 0x10; i<<=1) {
                    if (!(segmentMask & i)) thistime += 1;
                  }
                  lostSegments += thistime;
                  printf("lost segments(%u), segmentMask(0x%x)\n", thistime, segmentMask);
                  packetError = true;
                }
                if (wp[FrameCount] - lastFrameNumber > 1) {
                  unsigned lost = wp[FrameCount] - lastFrameNumber - 1;
                  lostEvents += lost;
                  printf("lost %u event%s %u ", lost, lost > 1 ? "s" : "", lastFrameNumber + 1);
                  if (lost > 1) printf("... %u", lastFrameNumber + lost);
                  printf("\n");
                  packetError = true;
                }
              }
              segmentMask = 0;
              lastFrameNumber = wp[FrameCount];
              if (!(lastFrameNumber%1000)) {
                printSummary();
                packetError = false;
              }
            }
            segmentMask |= 1 << wp[QuadNumber];
            if (packetError) {
              time_t mytime;
              mytime = time(NULL);
              printf(ctime(&mytime));
              printSummary();
            }
          }
        } else {
          printData( vcFetchBuffer, iRecvDataSize, sTyDump, cntDump, offDump, showHdr, quiet );
        }
      }
    }
  }
  return 0;
}

void printSummary()
{
  printf("packetCount(%llu), eventCount(%u), lostPackets(%u), lostSegments(%u), lostEvents(%u)\n",
      packetCount, lastFrameNumber, lostPackets, lostSegments, lostEvents);
}

using namespace	Pds;

void printData(
	const std::vector<unsigned char>& vcBuffer,
	int				iDataSize,
	char		*	sTyDump,
	size_t			cntDump,
	size_t			offDump,
	bool			showHdr,
  	bool			quiet
	)
{
  const unsigned char* pcData       = (const unsigned char*) &vcBuffer[0];

  static int iFiducialPrev     = -1;
  static int iFiducialDiffPrev = -1;
  int        iFiducialCurr     = 0x1ffff & *(unsigned int*)&pcData[12];

  static unsigned int iCount = 0;

  if ( iFiducialPrev != -1 && iFiducialCurr > 3 )
  {
    int iFiducialDiffCurr = iFiducialCurr - iFiducialPrev;
    if ( iFiducialDiffCurr != iFiducialDiffPrev )
		quiet = false;

	if ( !quiet )
    	printf( "** Size %d Fiducial Curr %d (0x%x) Diff %d", iDataSize, iFiducialCurr, iFiducialCurr,
				iFiducialDiffCurr );

    iFiducialDiffPrev = iFiducialDiffCurr;
  }
  else if ( iCount == 0 )
  {
    printf( "Fiducial Curr %d (0x%x)", iFiducialCurr, iFiducialCurr );
  }

  iFiducialPrev = iFiducialCurr;

  if ( !quiet )
  {
	enum DataDump_t	tyDump	= TY_DUMP_NONE;
	if ( sTyDump != NULL )
	{
	  if (		strcmp( sTyDump, "int"		) == 0 ) tyDump = TY_DUMP_INT;
	  else if (	strcmp( sTyDump, "uint"		) == 0 ) tyDump = TY_DUMP_UINT;
	  else if (	strcmp( sTyDump, "double"	) == 0 ) tyDump = TY_DUMP_DOUBLE;
	  else printf( "Dump Type %s not supported\n", sTyDump );
	}
	if ( tyDump != TY_DUMP_NONE )
	{
	  const uint32_t	* pData       = (const uint32_t * ) &vcBuffer[0];

	  if ( showHdr )
	  {
	  	const uint32_t	*	pUint       = (const uint32_t	* ) pData;
	  	uint32_t			uVal;
		uVal = *pUint++; printf( "\n uNanoSecs:   %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n uSecs:       %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n uMBZ1:       %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n uFidId:      %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n uMBZ2:       %10u (0x%08X)", uVal, uVal );

		uVal = *pUint++; printf( "\n XTC1 Damage: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC1 Log ID: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC1 PhysID: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC1 TyData: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC1 ExtSiz: %10u (0x%08X)", uVal, uVal );

		uVal = *pUint++; printf( "\n XTC2 Damage: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC2 Log ID: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC2 PhysID: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC2 TyData: %10u (0x%08X)", uVal, uVal );
		uVal = *pUint++; printf( "\n XTC2 ExtSiz: %10u (0x%08X)", uVal, uVal );

		printf( "\n" );
	  }
	  else
	  {
		  // Skip the 15 word header and requested dump offset (4 byte aligned)
		  pData += 15;
	  }
	  // Skip the 15 word header and requested dump offset (4 byte aligned)
	  pData += offDump/sizeof(uint32_t);

	  const int			* pInt        = (const int		* ) pData;
	  const uint32_t	* pUint       = (const uint32_t	* ) pData;
	  const double		* pDouble     = (const double	* ) pData;
	  printf( " Data: " );
	  while ( cntDump > 0 )
	  {
	    if ( (const unsigned char *)pData >= &vcBuffer[iDataSize] )
		{
			printf( " <EndOfData>" );
			break;
		}
		switch ( tyDump )
		{
		case TY_DUMP_NONE:
			printf( " ---" );
			break;
		case TY_DUMP_INT:
			printf( " %8d", *pInt++ );
	  		pData = (const uint32_t *) pInt;
			break;
		case TY_DUMP_UINT:
			printf( " %8u", *pUint++ );
	  		pData = (const uint32_t *) pUint;
			break;
		case TY_DUMP_DOUBLE:
			printf( " %.6e", *pDouble++ );
	  		pData = (const uint32_t *) pDouble;
			break;
		}
		cntDump--;
	  }
	}
	printf("\n");
	fflush(NULL);
	fsync(1);
  }

  //printf("Dumping Data (Data Size = %d):\n", iDataSize);
  //for (int i=0; i< iDataSize; i++)
  //{
  //    if ( pcData[i] >= 32 && pcData[i] <= 126 )
  //        printf( "%c", pcData[i] );
  //    else
  //        printf( "[%02d: %02X]", i, (unsigned int) pcData[i] );
  //}
  //printf("\n");
  iCount = (iCount+1) % 1000;

}
