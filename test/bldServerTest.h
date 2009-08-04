#ifndef BLD_SERVER_TEST_H
#define BLD_SERVER_TEST_H

#include <string>

namespace Pds 
{

class BldServerSlim 
{
public:
    BldServerSlim(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize, 
      char* sInterfaceIp = NULL);
    ~BldServerSlim();

    int fetch(unsigned int iBufSize, void* pFetchBuffer, unsigned int& iRecvDataSize);
    
private:
    static std::string addressToStr( unsigned int uAddr );
    
    int _iSocket;
    unsigned int _uMaxDataSize;
    unsigned int _uAddr, _uPort;
    
    struct msghdr      _hdr; // Control structure socket receive 
    struct iovec       _iov; // Buffer description socket receive 
    struct sockaddr_in _src; // Socket name source machine
};

}

#endif

