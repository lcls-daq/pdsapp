#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "multicastConfig.hh"
#include "bldClientTest.hh"

using std::string;
using namespace Pds::ConfigurationMulticast;

namespace EpicsBld
{
static int iDefaultDataSeed = 1;

/**
 * class Ins
 *
 * imported from ::Pds::Ins
 */
class Ins
{
public:
  Ins();
  enum Option {DoNotInitialize};
  explicit Ins(Option);
  explicit Ins(unsigned short port);
  explicit Ins(int address);
  Ins(int address, unsigned short port);
  Ins(const Ins& ins, unsigned short port);
  explicit Ins(const sockaddr_in& sa);

  int                   address()     const; 
  void                  address(int address);
  unsigned short        portId()      const; 

  int operator==(const Ins& that)  const;

protected:
  int      _address;
  unsigned _port;
};

/**
 * class Sockaddr
 *
 * imported from ::Pds::Sockaddr
 */

class Sockaddr {
public:
  Sockaddr() {
    _sockaddr.sin_family      = AF_INET;
}
  
  Sockaddr(const Ins& ins) {
    _sockaddr.sin_family      = AF_INET;
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId()); 
}
  
  void get(const Ins& ins) {
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId());     
}

  sockaddr* name() const {return (sockaddr*)&_sockaddr;}
  inline int sizeofName() const {return sizeof(_sockaddr);}

private:
  sockaddr_in _sockaddr;
};

/**
 * class Port
 *
 * imported from ::Pds::Port
 */
class Port : public Ins
{
  public:
    enum Type {ClientPort = 0, ServerPort = 2, VectoredServerPort = 3};
    ~Port();
    Port(Port::Type, 
            const Ins&, 
            int sizeofDatagram, 
            int maxPayload,
            int maxDatagrams);
    Port(Port::Type, 
            int sizeofDatagram, 
            int maxPayload,
            int maxDatagrams);
    int  sizeofDatagram() const;
    int  maxPayload()     const;
    int  maxDatagrams()   const;
    Type type()           const;

    // Construction may potentially fail. If the construction is
    // successful error() returns 0. If construction fails, error()
    // returns a non-zero value which corresponds to the reason (as an
    // "errno").
    int   error() const;
    int   getSocket()  { return _socket; } // modified by tomy, used to get the socket handle
  protected:
    void error(int value);
    int _socket;
  private:
    enum {Class_Server = 2, Style_Vectored = 1};
    void   _open(Port::Type, 
     const Ins&, 
     int sizeofDatagram, 
     int maxPayload,
     int maxDatagrams);
    int    _bind(const Ins&);
    void   _close();
    Type   _type;
    int    _sizeofDatagram;
    int    _maxPayload;
    int    _maxDatagrams;
    int    _error;
};

class Client : public Port
{
  public:
    Client(int sizeofDatagram,
     int maxPayload,
     int maxDatagrams = 1);
    
    ~Client();

  public:
    int send(const char* datagram, const char* payload, int sizeofPayload, const Ins&);
     
    /*
   * Multicast functions
   */
    void multicastSetInterface(unsigned interface); 
  int multicastSetTTL(unsigned char ucTTL);
     
  private:
     enum {SendFlags = 0};
#ifdef ODF_LITTLE_ENDIAN
    // Would prefer to have _swap_buffer in the stack, but it triggers
    // a g++ bug (tried release 2.96) with pointers to member
    // functions of classes which are bigger than 0x7ffc bytes; this
    // bug appeared in Outlet which points to OutletWire member
    // functions; hence OutletWire must be <= 0x7ffc bytes; if
    // Client is too large the problem appears in OutletWire
    // since it contains an Client. 
    // Later note: g++ 3.0.2 doesn't show this problem.
    char* _swap_buffer;
    void _swap(const iovec* iov, unsigned msgcount, iovec* iov_swap);
#endif
};

/**
 * Bld Multicast Client Test class
 */
class BldClientTest : public BldClientInterface
{
public:
  BldClientTest(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize,
    unsigned char ucTTL = 32, char* sInteraceIp = NULL);
  BldClientTest(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize,
    unsigned char ucTTL = 32, unsigned int uInteraceIp = 0);
  virtual ~BldClientTest();
  
  virtual int sendRawData(int iSizeData, const char* pData);
private:
  int _initClient( unsigned int uMaxDataSize, unsigned char ucTTL, unsigned int uInterfaceIp);
  int _initClient( unsigned int uMaxDataSize, unsigned char ucTTL, char* sInterfaceIp);

  Client* _pSocket; // socket
  unsigned _uAddr, _uPort;
};

static string addressToStr( unsigned int uAddr )
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

} // namespace EpicsBld


extern "C" 
{ 
// forward declaration
int BldClientTestSendInterface(int iDataSeed, char* sInterfaceIp);
int BldClientTestSendAddr(unsigned int uAddr, unsigned int uPort, 
      unsigned int uMaxDataSize, int iDataSeed, char* sInterfaceIp);
  
/**
 * Bld Client basic test function
 *
 * Will continuously send out the multicast packets to a default
 * address with default values. Need to be stop manually from keyboard
 * by pressing Ctrl+C
 *
 * This fucntion is only used for quick testing of Bld Client, such as 
 * running from CExp Command Line.
 */
int BldClientTestSendBasic(int iDataSeed)
{ 
  return BldClientTestSendInterface(iDataSeed, NULL);
}

/**
 * Bld Client test function with IP interface selection
 *
 * Similar to the above function. It lets user to specify the network interface by
 * IP address (xxx.xx.xx.xx) or name (eth0, eth1,...).
 *
 * This fucntion is only used for quick testing of Bld Client, such as 
 * running from CExp Command Line.
 */
int BldClientTestSendInterface(int iDataSeed, char* sInterfaceIp)
{ 
  return BldClientTestSendAddr(uDefaultAddr, uDefaultPort, uDefaultMaxDataSize, iDataSeed, sInterfaceIp);
}

/**
 * Bld Client test function with address and IP interface selection
 *
 * Similar to BldClientTestSendBasic(), but with the argument (sInterfaceIp)
 * to specify the IP interface for sending multicast.
 *
 * Will continuously send out the multicast packets to a default
 * address with default values. Need to be stop manually from keyboard
 * by pressing Ctrl+C
 *
 * This fucntion is only used for quick testing of Bld Client, such as 
 * running from CExp Command Line.
 */
int BldClientTestSendAddr(unsigned int uAddr, unsigned int uPort, 
      unsigned int uMaxDataSize, int iDataSeed, char* sInterfaceIp)
{
  const int iSleepInterval = 3;
      
  EpicsBld::BldClientInterface* pBldClient = 
    EpicsBld::BldClientFactory::createBldClient(uAddr, uPort, 
      uMaxDataSize, ucDefaultTTL, sInterfaceIp);

  unsigned int uIntDataSize = (uMaxDataSize/sizeof(int));
  int* liData = new int[uIntDataSize];
  int iTestValue = iDataSeed * 1000;

  printf( "Beginning Multicast Client Testing\n"
          "Data Seed = %d  Interface = %s  Buffer Size = %u\n"
          "Press Ctrl+C to Exit...\n",
          iDataSeed, 
          (sInterfaceIp ? sInterfaceIp: "NULL [Default Interface]"),
          uMaxDataSize );  
  
  while ( 1 )  
  {
    for (unsigned int uIndex=0; uIndex<uIntDataSize; uIndex++)
      liData[uIndex] = iTestValue;
      
    printf("Bld send to %s port %d Value %d\n", EpicsBld::addressToStr(uAddr).c_str(), uPort, iTestValue);
    
    pBldClient->sendRawData(uIntDataSize*sizeof(int), 
      reinterpret_cast<char*>(liData));
    iTestValue++;
    sleep(iSleepInterval);
    // Waiting for keyboard interrupt to break the infinite loop
  } 
  
  delete[] liData;
  delete pBldClient;
  
  return 0;
}

/* 
 * The following functions provide C wrappers for accesing EpicsBld::BldClientInterface
 * and EpicsBld::BldClientFactory
 */

/**
 * Init function: Use EpicsBld::BldClientFactory to construct the BldClient
 * and save the pointer in (*ppVoidBldClient)
 */ 
int BldClientInitByInterfaceName(unsigned uAddr, unsigned uPort, 
  unsigned int uMaxDataSize, unsigned char ucTTL, char* sInterfaceIp, 
  void** ppVoidBldClient)
{
  if ( ppVoidBldClient == NULL )
    return 1;
  
  EpicsBld::BldClientInterface* pBldClient = 
    EpicsBld::BldClientFactory::createBldClient(uAddr, uPort, 
      uMaxDataSize, ucTTL, sInterfaceIp);
    
  *ppVoidBldClient = reinterpret_cast<void*>(pBldClient);
  
  return 0;
}

int BldClientInitByInterfaceAddress(unsigned uAddr, unsigned uPort, 
  unsigned int uMaxDataSize, unsigned char ucTTL, unsigned int uInterfaceIp, 
  void** ppVoidBldClient)
{
  if ( ppVoidBldClient == NULL )
    return 1;
  
  EpicsBld::BldClientInterface* pBldClient = 
    EpicsBld::BldClientFactory::createBldClient(uAddr, uPort, 
      uMaxDataSize, ucTTL, uInterfaceIp);
    
  *ppVoidBldClient = reinterpret_cast<void*>(pBldClient);
  
  return 0;
}

/**
 * Release function: Call C++ delete operator to delete the BldClient
 */
int BldClientRelease(void* pVoidBldClient)
{
  if ( pVoidBldClient == NULL )
    return 1;
    
  EpicsBld::BldClientInterface* pBldClient = 
    reinterpret_cast<EpicsBld::BldClientInterface*>(pVoidBldClient);
  delete pBldClient;
    
  return 0;
}

/**
 * Call the Send function defined in EpicsBld::BldClientInterface 
 */
int BldClientSendRawData(void* pVoidBldClient, int iSizeData, char* pData)
{
  if ( pVoidBldClient == NULL || pData == NULL )
    return -1;

  EpicsBld::BldClientInterface* pBldClient = 
    reinterpret_cast<EpicsBld::BldClientInterface*>(pVoidBldClient);    

  return pBldClient->sendRawData(iSizeData, pData);
}

static void showUsage()
{
    printf( "Usage:  bldClientTest  [-v|--version] [-h|--help] [-a|--address <Ip Address>] [-p|--port <Port>] [-i|--interface <Interface Name/IP>]  [<Data Seed> <Interface name/IP>]\n" 
      " Options:\n"
      "   -v|--version                        Show file version\n"
      "   -h|--help                           Show Usage\n"
      "   -a|--address   <Ip Address>         Set the multicast address to be sent to. Default: %s\n"
      "   -p|--port      <Port>               Set the port to be sent to. Default: %u\n"
      "   -s|--seed      <Data Seed>          Set the seed value for generating different data for each client program. Default: %i\n"
      "   -z|--size      <Buffer Size>        Set the max data size for allocating buffer. Default: %u\n"
      "   -i|--interface <Interface Name/IP>  Set the network interface for receiving multicast. Use ether IP address (xxx.xx.xx.xx) or name (eth0, eth1,...)\n"
      "   <Data Seed>                         Same as -s flag above. *This is an argument without the option (-s) flag\n"
      "   <Interface Name/IP>                 Same as -i flag above. *This is an argument without the option (-i) flag\n",
      EpicsBld::addressToStr(uDefaultAddr).c_str(), uDefaultPort, EpicsBld::iDefaultDataSeed, uDefaultMaxDataSize
    );
    
}

static const char sBldClientTestVersion[] = "0.90";

static void showVersion()
{
    printf( "Version:  bldClientTest  Ver %s\n", sBldClientTestVersion );
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
     {"seed",     1, 0, 's'},
     {"size",     1, 0, 'z'},
     {"interface",1, 0, 'i'},            
     {0,          0, 0,  0  }
  };    
    
  unsigned int  uAddr         = uDefaultAddr;
  unsigned int  uPort         = uDefaultPort;
  unsigned int  uMaxDataSize  = uDefaultMaxDataSize;  
  int           iDataSeed     = EpicsBld::iDefaultDataSeed;
  char*         sInterfaceIp  = NULL;      
  while ( int opt = getopt_long(argc, argv, ":vha:p:i:s:z:", loOptions, &iOptionIndex ) )
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
          iDataSeed = strtol(optarg, NULL, 0);
          break;
      case 'z':
          uMaxDataSize = strtoul(optarg, NULL, 0);
          break;
      case 'i':
          sInterfaceIp = optarg;
          break;            
      case '?':               /* Terse output mode */
          printf( "bldClientTest:main(): Unknown option: %c\n", optopt );
          break;
      case ':':               /* Terse output mode */
          printf( "bldClientTest:main(): Missing argument for %c\n", optopt );
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
    iDataSeed = atoi(argv[0]);
  
  if (argc >= 2 )    
    sInterfaceIp = argv[1];
    
  return BldClientTestSendAddr( uAddr, uPort, uMaxDataSize, iDataSeed, sInterfaceIp );
}

} // extern "C" 

/**
 * class member definitions
 *
 */
namespace EpicsBld
{
/**
 * class Ins
 */
inline int Ins::operator==(const Ins& that) const{
  return (_address == that._address && _port == that._port);
}

inline Ins::Ins(Option) 
{
}

inline Ins::Ins() 
{
  _address = INADDR_ANY;
  _port    = 0;
}

inline Ins::Ins(unsigned short port) 
{
    _address = INADDR_ANY;
    _port    = port;
}

inline Ins::Ins(int address, unsigned short port) 
{
    _address = address;
    _port    = port;
}

inline Ins::Ins(const Ins& ins, unsigned short port) 
{
    _address = ins._address;
    _port    = port;
}

inline Ins::Ins(const sockaddr_in& sa) 
{
    _address = ntohl(sa.sin_addr.s_addr);
    _port    = ntohs(sa.sin_port);
}

inline Ins::Ins(int address) 
{
    _address = address;
    _port    = 0;
}

inline unsigned short Ins::portId() const 
{
    return _port;  
}

inline void Ins::address(int address) 
{
    _address = address;
}

inline int Ins::address() const 
{
    return _address;
} 

/**
 * class Port
 */
inline Port::~Port() 
{
  _close();
}

inline Port::Port(Port::Type type, 
                        int sizeofDatagram, 
                        int maxPayload,
                        int maxDatagrams) : 
  Ins(Ins::DoNotInitialize)
{
  Ins ins; _open(type, ins, sizeofDatagram, maxPayload, maxDatagrams);
}

inline Port::Port(Port::Type type, 
           const Ins& ins, 
           int sizeofDatagram, 
           int maxPayload,
           int maxDatagrams) : 
Ins(Ins::DoNotInitialize)
{
_open(type, ins, sizeofDatagram, maxPayload, maxDatagrams);
}

inline Port::Type Port::type() const
{
  return _type;
}

inline int Port::sizeofDatagram() const
{
  return _sizeofDatagram;
}

inline int Port::maxPayload() const
{
  return _maxPayload;
}


inline int Port::maxDatagrams() const
{
  return _maxDatagrams;
}

inline int Port::error() const
{
  return _error;
}

inline void Port::error(int value)
{
  _error = value;
}

void Port::_open(Port::Type type,
                    const Ins& ins,
                    int sizeofDatagram,
                    int maxPayload,
                    int maxDatagrams)
{
  _socket         = socket(AF_INET, SOCK_DGRAM, 0);
  _sizeofDatagram = sizeofDatagram;
  _maxPayload     = maxPayload;
  _maxDatagrams   = maxDatagrams;
  _type           = type;
  if(!(_error = (_socket != -1) ? _bind(ins) : errno))
    {
      sockaddr_in name;
#ifdef VXWORKS
      int length = sizeof(name);
#else
      socklen_t length = sizeof(name);
#endif
      if(getsockname(_socket, (sockaddr*)&name, &length) == 0) {
  _address = ntohl(name.sin_addr.s_addr);
  _port = (unsigned)ntohs(name.sin_port);
      } else {
  _error = errno;
  _close();
      }
    }
  else
    _close();
  if (_error) {
    printf("*** Port failed to open address 0x%x port %i: %s\n", 
     ins.address(), ins.portId(), strerror(errno));
}
}



int Port::_bind(const Ins& ins)
{
  int s = _socket;

  int yes = 1;
  if(setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes)) == -1)
    return errno;
  
//  struct ip_mreq  mreq;

//  mreq.imr_multiaddr.s_addr = 239<<24 | 255<<16 | 0<<8 | 1;
//  mreq.imr_interface.s_addr = 172<<24 | 21<<16 | 9<<8 | 23;
//  if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                        //sizeof(mreq)) == -1 )
//    printf( "Error adding multicast group\n" );
  

  if(_type == Port::ClientPort)
    {
    int parm = (_sizeofDatagram + _maxPayload + sizeof(struct sockaddr_in)) *
               _maxDatagrams;

#ifdef VXWORKS                           // Out for Tornado II 7/21/00 - RiC
    // The following was found exprimentally with ~claus/bbr/test/sizeTest
    // The rule may well change if the mBlk, clBlk or cluster parameters change
    // as defined in usrNetwork.c - RiC
    parm = parm + (88 + 32 * ((parm - 1993) / 2048));
#endif

    if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&parm, sizeof(parm)) == -1)
      return errno;
    }

  if(_type & Class_Server)
    {
    int parm = (_sizeofDatagram + _maxPayload + sizeof(struct sockaddr_in)) *
               _maxDatagrams;
#ifdef __linux__
    parm += 2048; // 1125
#endif
    if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&parm, sizeof(parm)) == -1)
      return errno;
    if(ins.portId() && (_type & Style_Vectored))
      {
      int y = 1;
#ifdef VXWORKS 
      if(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char*)&y, sizeof(y)) == -1)
#else
      if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1)
#endif
        return errno;
      }
    }

#ifdef VXWORKS
  Sockaddr sa(Ins(ins.portId()));
#else
  Sockaddr sa(ins);
#endif
  return (bind(s, sa.name(), sa.sizeofName()) == -1) ? errno : 0;
}

void Port::_close()
{
  if(_socket != -1)
    {
    close(_socket);

    _socket = -1;
    }
}

/**
 * class Client
 */

Client::Client(int sizeofDatagram,
                     int maxPayload,
                     int maxDatagrams) :
  Port(Port::ClientPort,
          sizeofDatagram,
          maxPayload,
          maxDatagrams)
{
#ifdef ODF_LITTLE_ENDIAN
    _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif
}

/*
** ++
**
**    Delete swap buffer under little endian machines
**
** --
*/


Client::~Client() {
#ifdef ODF_LITTLE_ENDIAN
  delete _swap_buffer;
#endif
}

/*
** ++
**
**    Use specified interface for sending multicast addresses
**
** --
*/

void Client::multicastSetInterface(unsigned interface)
{
  in_addr address;
  address.s_addr = htonl(interface);
  if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address,
                 sizeof(in_addr)) < 0) 
  {
    printf( "Client::multicastSetInterface():  setsockopt(IP_MULTICAST_IF, ...) failed\n"
     "  errno = %d (%s)\n", errno, strerror(errno) );
    error(errno);
  }
}

/**
 * Set the multicast TTL value
 *
 * @param ucTTL  The TTL value
 * @return 0 if successful, 1 if error 
 */
int Client::multicastSetTTL(unsigned char ucTTL)
{
  if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ucTTL,
         sizeof(ucTTL)) < 0) 
    return 1;
  
  return 0; 
}

/*
** ++
**
**    This function is used to transmit the specified datagram to the
**    address specified by the "dst" argument.
**    The datagram is specified in two parts: "datagram", which is a
**    pointer the datagram's fixed size header, and "payload", which
**    is a pointer to the datagrams' variable sized portion. The size
**    of the payload (in bytes) is specified by the "sizeofPayload"
**    argument. The function returns the transmission status. A value
**    of zero (0) indicates the transfer was sucessfull. A positive
**    non-zero value is the reason (as an "errno"value) the transfer
**    failed.
**
** --
*/


int Client::send(const char*         datagram,
                 const char*         payload,
                    int           sizeofPayload,
                    const Ins& dst)
{
    struct msghdr hdr;
    struct iovec  iov[2];

  if(datagram)
    {
    iov[0].iov_base = (caddr_t)(datagram);
    iov[0].iov_len  = sizeofDatagram();
    iov[1].iov_base = (caddr_t)(payload);
    iov[1].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 2;
    }
  else
    {
    iov[0].iov_base = (caddr_t)(payload);
    iov[0].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 1;
    }

  Sockaddr sa(dst);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;


#ifdef ODF_LITTLE_ENDIAN
  struct iovec iov_swap;
  _swap(iov, hdr.msg_iovlen, &iov_swap);
  hdr.msg_iov       = &iov_swap;
  hdr.msg_iovlen    = 1;
#else
  hdr.msg_iov       = &iov[0];
#endif

  int length = sendmsg(_socket, &hdr, SendFlags);
  return (length == - 1) ? errno : 0;
}

#ifdef ODF_LITTLE_ENDIAN
void  Client::_swap(const iovec* iov, unsigned msgcount, iovec* iov_swap) {
  unsigned* dst = (unsigned*)_swap_buffer;  
  const iovec* iov_end = iov+msgcount;
  unsigned total = 0;
  do {
    unsigned  len = iov->iov_len;
    unsigned* src = (unsigned*)(iov->iov_base);
    unsigned* end = src + (len >> 2);
    while (src < end) *dst++ = htonl(*src++);
    total += len;
} while (++iov < iov_end);
  iov_swap->iov_len = total;
  iov_swap->iov_base = _swap_buffer; 
}
#endif

/**
 * class BldClientFactory
 */
BldClientInterface* BldClientFactory::createBldClient(unsigned uAddr, 
    unsigned uPort, unsigned int uMaxDataSize, unsigned char ucTTL, 
    char* sInteraceIp)
{
  return new BldClientTest(uAddr, uPort, uMaxDataSize, ucTTL, sInteraceIp );
}

BldClientInterface* BldClientFactory::createBldClient(unsigned uAddr, 
    unsigned uPort, unsigned int uMaxDataSize, unsigned char ucTTL, 
    unsigned int uInterfaceIp)
{
  return new BldClientTest(uAddr, uPort, uMaxDataSize, ucTTL, uInterfaceIp );
}

/**
 * class BldClientTest
 */
BldClientTest::BldClientTest(unsigned uAddr, unsigned uPort, 
  unsigned int uMaxDataSize, unsigned char ucTTL, char* sInterfaceIp) : 
  _pSocket(NULL), _uAddr(uAddr), _uPort(uPort)
{
  _initClient(uMaxDataSize, ucTTL, sInterfaceIp); 
}

BldClientTest::BldClientTest(unsigned uAddr, unsigned uPort, 
  unsigned int uMaxDataSize, unsigned char ucTTL, unsigned int uInterfaceIp) : 
  _pSocket(NULL), _uAddr(uAddr), _uPort(uPort)
{ 
  _initClient(uMaxDataSize, ucTTL, uInterfaceIp );
}

int BldClientTest::_initClient( unsigned int uMaxDataSize, unsigned char ucTTL, unsigned int uInterfaceIp)
{
  if ( _pSocket != NULL) delete _pSocket; 
  
  _pSocket = new Client(0, uMaxDataSize);
  _pSocket->multicastSetTTL(ucTTL);
  
  if ( uInterfaceIp != 0) 
  {
    printf( "multicast interface IP: %s\n", EpicsBld::addressToStr(uInterfaceIp).c_str() );
    _pSocket->multicastSetInterface(uInterfaceIp);
  }   
  return 0;
}

int BldClientTest::_initClient( unsigned int uMaxDataSize, unsigned char ucTTL, char* sInterfaceIp)
{
  if ( _pSocket != NULL) delete _pSocket; 
  
  _pSocket = new Client(0, uMaxDataSize);
  _pSocket->multicastSetTTL(ucTTL);
    
  in_addr_t uiInterface;
   
  if (sInterfaceIp == NULL || sInterfaceIp[0] == 0 )
  {
    uiInterface = 0;
  }
  else
  {
      if ( sInterfaceIp[0] < '0' || sInterfaceIp[0] > '9' )
      {        
        struct ifreq ifr;
        strcpy( ifr.ifr_name, sInterfaceIp );
        int iError = ioctl( _pSocket->getSocket(), SIOCGIFADDR, (char*)&ifr );
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
    
  if ( uiInterface != 0) 
  {
    printf( "multicast interface IP: %s\n", EpicsBld::addressToStr(uiInterface).c_str() );
    _pSocket->multicastSetInterface(uiInterface);
  }   
  return 0;
}

BldClientTest::~BldClientTest() 
{
  if ( _pSocket != NULL) delete _pSocket;
}

int BldClientTest::sendRawData(int iSizeData, const char* pData)
{
  Ins insDst( _uAddr, _uPort );
  return _pSocket->send(NULL, pData, iSizeData, insDst);
}

} // namespace EpicsBld
