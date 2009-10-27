#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <getopt.h>
#include <string>
#include <vector>
#include <sstream>
#include <sys/ioctl.h>
#include <net/if.h>

#include "pds/service/NetServer.hh"
#include "pds/service/Ins.hh"
#include "multicastConfig.hh"
#include "bldServerTest.h"

using std::string;

namespace Pds 
{
  
static std::string addressToStr( unsigned int uAddr );

BldServerSlim::BldServerSlim(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize,
  char* sInterfaceIp) : _iSocket(-1), _uMaxDataSize(uMaxDataSize), _uAddr(uAddr), _uPort(uPort), _bInitialized(false)
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
void printData( const std::vector<unsigned char>& vcBuffer, int iDataSize );

static void showUsage()
{
    printf( "Usage:  bldServerTest  [-v|--version] [-h|--help] [-a|--address <Ip Address>] [-p|--port <Port>] [-i|--interface <Interface Name/IP>]  [<Interface name/IP>]\n"
      " Options:\n"
      "   -v|--version                        Show file version\n"
      "   -h|--help                           Show Usage\n"
      "   -a|--address   <Ip Address>         Set the multicast address to be listened to. Default: %s\n"
      "   -p|--port      <Port>               Set the port to be listened to. Default: %u\n"
      "   -s|--size      <Buffer Size>        Set the max data size for allocating buffer. Default: %u\n"
      "   -i|--interface <Interface Name/IP>  Set the network interface for receiving multicast. Use ether IP address (xxx.xx.xx.xx) or name (eth0, eth1,...)\n"
      "   <Interface IP/Name>                 Same as above. *This is an argument without the option (-i) flag\n",
      Pds::addressToStr(uDefaultAddr).c_str(), uDefaultPort, uDefaultMaxDataSize
    );
}

static const char sBldServerTestVersion[] = "0.90";

static void showVersion()
{
    printf( "Version:  bldServerTest  Ver %s\n", sBldServerTestVersion );
}

int main(int argc, char** argv)
{
    int iOptionIndex = 0;
    struct option loOptions[] = 
    {
       {"ver",      0, 0, 'v'},
       {"help",     0, 0, 'h'},
       {"address",  1, 0, 'a'},
       {"port",     1, 0, 'p'},
       {"size",     1, 0, 's'},
       {"interface",1, 0, 'i'},       
       {0,          0, 0,  0 }
    };    
      
    char*         sInterfaceIp  = NULL;    
    unsigned int  uAddr         = uDefaultAddr;
    unsigned int  uPort         = uDefaultPort;
    unsigned int  uMaxDataSize  = uDefaultMaxDataSize;
    while ( int opt = getopt_long(argc, argv, ":vha:p:i:s:", loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt) 
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;
        case 'a':
            uAddr = ntohl(inet_addr(optarg));
            break;
        case 'p':
            uPort = strtoul(optarg, NULL, 0);
            break;
        case 's':
            uMaxDataSize = strtoul(optarg, NULL, 0);
            break;
        case 'i':
            sInterfaceIp = optarg;
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
          
    if (argc >= 1 )
      sInterfaceIp = argv[0];    
    
    Pds::BldServerSlim bldServer(uAddr, uPort, uMaxDataSize, sInterfaceIp ); 
    if ( !bldServer.IsInitialized() )
    {
      printf( "bldServerTest:main(): bldServerSlim is not initialzed successfully\n" );
      return 1;
    }
            
    std::vector<unsigned char> vcFetchBuffer( uDefaultMaxDataSize );
        
    printf( "Beginning Multicast Server Testing. Press Ctrl-C to Exit...\n" );        
    
    while (1) // Pds::ConsoleIO::kbhit(NULL) == 0 )
    {       
        unsigned int iRecvDataSize = 0;
        int iFail = bldServer.fetch(uDefaultMaxDataSize, &vcFetchBuffer[0], iRecvDataSize);
        if ( iFail != 0 )
        {
          printf( "bldServerTest:main(): bldServer.fetch() failed. Error Code = %d\n", iFail );
          return 2;
        }
        
        printData( vcFetchBuffer, iRecvDataSize );
    }

    return 0;
}

void printData( const std::vector<unsigned char>& vcBuffer, int iDataSize )
{
    const unsigned char* pcData = (const unsigned char*) &vcBuffer[0];    
    
    printf("Dumping Data (Data Size = %d):\n", iDataSize);
    for (int i=0; i< iDataSize; i++)
    {
//        if ( pcData[i] >= 32 && pcData[i] <= 126 )
//            printf( "%c", pcData[i] );
//        else
            printf( "[%02d: %02X]", i, (unsigned int) pcData[i] );           
    }
    printf("\n");
}
