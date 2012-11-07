#ifndef BLD_SERVER_TEST_H
#define BLD_SERVER_TEST_H

#include <string>
#include "pds/xtc/EvrDatagram.hh"

namespace Pds 
{

  class BldServerSlim
  {
    public:
      BldServerSlim(unsigned uAddr, unsigned uPort, unsigned int uMaxDataSize,
          char* sInterfaceIp = NULL);
      ~BldServerSlim();

      bool IsInitialized() { return _bInitialized; }
      int fetch(unsigned int iBufSize, void* pFetchBuffer, unsigned int& iRecvDataSize);
      unsigned lastEVR() { return _lastEvr; }
      void lastEVR(unsigned);
      void thisDgram(Pds::EvrDatagram*);

    private:
      int               _iSocket;
      unsigned int      _uMaxDataSize;
      unsigned int      _uAddr, _uPort;
      bool              _bInitialized;
      unsigned          _lastEvr;
      Pds::EvrDatagram* _lastDgram;
      Pds::EvrDatagram  _myDgram;
      unsigned          _fiducialGap;

      struct msghdr      _hdr; // Control structure socket receive
      struct iovec       _iov; // Buffer description socket receive
      struct sockaddr_in _src; // Socket name source machine
  };

}

#endif

