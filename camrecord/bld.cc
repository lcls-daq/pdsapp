#include<sstream>
#include<net/if.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/ioctl.h>
#include<sys/errno.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<vector>

#include"yagxtc.hh"

using namespace std;

// Code shamelessly stolen from bldServerTest.{cpp,h}
static const unsigned int uDefaultAddr = 239<<24 | 255<<16 | 24<<8; /// multicast address without low byte
static const unsigned int uDefaultPort = 10148;
static const unsigned int uDefaultMaxDataSize = 512; /// in bytes

class BldServerSlim 
{
public:
    BldServerSlim(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize, 
      char* sInterfaceIp = NULL);
    ~BldServerSlim();

    bool IsInitialized() { return _bInitialized; }
    int fd() { return _iSocket; }
    int fetch(unsigned int iBufSize, void* pFetchBuffer, unsigned int& iRecvDataSize);
    
private:    
    int           _iSocket;
    unsigned int  _uMaxDataSize;
    unsigned int  _uAddr, _uPort;
    bool          _bInitialized;
    
    struct msghdr      _hdr; // Control structure socket receive 
    struct iovec       _iov; // Buffer description socket receive 
    struct sockaddr_in _src; // Socket name source machine
};

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

    if(getsockname(_iSocket, (sockaddr*)&sockaddrName, &iLength) != 0) 
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
    close(_iSocket);
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

/******************** END OF STOLEN CODE ********************/

#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/Dgram.hh"
#include"pdsdata/xtc/DetInfo.hh"
#include"pdsdata/xtc/ProcInfo.hh"
#include"pdsdata/xtc/TransitionId.hh"
#include"pdsdata/pulnix/TM6740ConfigV2.hh"
#include"pdsdata/opal1k/ConfigV1.hh"
#include"pdsdata/camera/FrameV1.hh"
#include"pdsdata/bld/bldData.hh"
#include"pds/service/NetServer.hh"
#include"pds/service/Ins.hh"

using namespace Pds;

class bldconn {
 public:
    bldconn(string _name, int _address, string _device, int _revtime)
        : cfgdone(0), revtime(_revtime), name(_name), address(_address | uDefaultAddr), device(_device) {
        id = conns.size();
        conns.push_back(this);
        bldServer = new BldServerSlim(address, uDefaultPort, uDefaultMaxDataSize,
                                      const_cast<char *>(device.c_str())); 
        if ( !bldServer->IsInitialized() ) {
            printf( "bldServerTest:main(): bldServerSlim is not initialzed successfully\n" );
            exit(0);
        }
        fd = bldServer->fd();
#if 0
        printf("BLD at address %d on device %s has fd %d\n", _address, device.c_str(), fd);
#endif
        add_socket(fd);
        xid = register_xtc(1, name);
    }
    static bldconn *index(int idx) {
        if (idx >= 0 && idx < (int) conns.size())
            return conns[idx];
        else
            return NULL;
    }
    ~bldconn(void) {
        delete bldServer;
    }
    int fd;
    int cfgdone;
    int id;
    int xid;
    int revtime;
    BldServerSlim *bldServer;
 private:
    static vector<bldconn *> conns;
    string name;
    int address;
    string device;
};

vector<bldconn *> bldconn::conns;

void initialize_bld(void)
{
    // We're good.
}

void create_bld(string name, int address, string device, int revtime)
{
    new bldconn(name, address, device, revtime);
}

void handle_bld(fd_set *rfds)
{
    int i;
    bldconn *c;
    char buf[uDefaultMaxDataSize];
    unsigned int msgsize;
    Dgram *dg2 = (Dgram *)buf;
    Xtc   *inner = (Xtc *)(sizeof(Xtc) + (char *) &dg2->xtc);

    for (i = 0, c = bldconn::index(i); c; c = bldconn::index(++i)) {
        if (FD_ISSET(c->fd, rfds)) {
            c->bldServer->fetch(sizeof(buf), buf, msgsize);
            // The first packet, we treat as configuration information.
            if (!c->cfgdone) {
                if (c->revtime)
                    configure_xtc(c->xid, (char *) inner, dg2->xtc.extent,
                                  dg2->seq.clock().nanoseconds(),
                                  (dg2->seq.clock().seconds() & ~0x1ffff) | dg2->seq.stamp().fiducials());
                else
                    configure_xtc(c->xid, (char *) inner, dg2->xtc.extent,
                                  dg2->seq.clock().seconds(),
                                  (dg2->seq.clock().nanoseconds() & ~0x1ffff) | dg2->seq.stamp().fiducials());
                c->cfgdone = 1;
            } else {
                // Make sure the BLD puts the fiducial in the timestamp!
                if (c->revtime)
                    data_xtc(c->xid, dg2->seq.clock().nanoseconds(),
                             (dg2->seq.clock().seconds() & ~0x1ffff) | dg2->seq.stamp().fiducials(),
                             inner, dg2->xtc.extent, NULL);
                else
                    data_xtc(c->xid, dg2->seq.clock().seconds(),
                             (dg2->seq.clock().nanoseconds() & ~0x1ffff) | dg2->seq.stamp().fiducials(),
                             inner, dg2->xtc.extent, NULL);
            }
        }
    }

}

void cleanup_bld(void)
{
    int i;
    bldconn *c;
    for (i = 0, c = bldconn::index(i); c; c = bldconn::index(++i))
        delete c;
}

