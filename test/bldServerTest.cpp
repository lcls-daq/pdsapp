#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <getopt.h>
#include <string>
#include <vector>
#include <sstream>

#include "pds/service/NetServer.hh"
#include "pds/service/Ins.hh"
#include "multicastConfig.hh"
#include "bldServerTest.h"

using std::string;

namespace Pds 
{

BldServerSlim::BldServerSlim(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize,
  char* sInterfaceIp) : _iSocket(-1), _uMaxDataSize(uMaxDataSize), _uAddr(uAddr), _uPort(uPort)
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
        printf( "Local addr: %s Port %u\n", addressToStr(uSockAddr).c_str(), uSockPort );
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
    if (sInterfaceIp != NULL)
        printf( "multicast interface IP: %s\n", sInterfaceIp );

    in_addr_t uiInterface =  (
      sInterfaceIp == NULL || sInterfaceIp[0] == 0? 
      INADDR_ANY :
      ntohl(inet_addr(sInterfaceIp)) );

    struct ip_mreq ipMreq;
    memset((char*)&ipMreq, 0, sizeof(ipMreq));
    ipMreq.imr_multiaddr.s_addr = htonl(_uAddr);
    ipMreq.imr_interface.s_addr = htonl(uiInterface);
    if (
      setsockopt (_iSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipMreq,
      sizeof(ipMreq)) < 0 )
        throw string("BldServerSlim::BldServerSlim() : setsockopt(...IP_ADD_MEMBERSHIP) failed");    
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
        
    _iov.iov_base = pFetchBuffer;
    _iov.iov_len  = iBufSize;               
    
    int iFlags = 0;
    iRecvDataSize = recvmsg(_iSocket, &_hdr, iFlags);
        
    return 0;
}

string BldServerSlim::addressToStr( unsigned int uAddr )
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
    printf( "Usage:  bldServerTest  [-v|--version] [-h|--help] <Interface IP>\n" 
      "  Options:\n"
      "    -v|--version       Show file version\n"
      "    -h|--help          Show Usage\n"
      "    <Interface IP>     Set the network interface for receiving multicast\n"
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
       {0,          0, 0,  0  }
    };    
        
    while ( int opt = getopt_long(argc, argv, ":vh", loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
            
        switch(opt) 
        {            
        case 'v':               /* Print usage */
            showVersion();
            return 0;            
        case '?':               /* Terse output mode */
            printf( "epicsArch:main(): Unknown option: %c\n", optopt );
            break;
        case ':':               /* Terse output mode */
            printf( "epicsArch:main(): Missing argument for %c\n", optopt );
            break;
        default:            
        case 'h':               /* Print usage */
            showUsage();
            return 0;
            
        }
    }

    argc -= optind;
    argv += optind;  
      
    char* sInterfaceIp = NULL;
    
    if (argc >= 1 )
      sInterfaceIp = argv[0];    
    
    Pds::BldServerSlim bldServer(uDefaultAddr, uDefaultPort, uDefaultMaxDataSize,
      sInterfaceIp ); 
            
    std::vector<unsigned char> vcFetchBuffer( uDefaultMaxDataSize );
        
    printf( "Beginning Multicast Server Testing. Press Ctrl-C to Exit...\n" );        
    
    while (1) // Pds::ConsoleIO::kbhit(NULL) == 0 )
    {       
        unsigned int iRecvDataSize = 0;
        bldServer.fetch(uDefaultMaxDataSize, &vcFetchBuffer[0], iRecvDataSize);
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
