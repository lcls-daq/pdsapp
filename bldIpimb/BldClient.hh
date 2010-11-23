#ifndef PDS_BLD_CLIENT
#define PDS_BLD_CLIENT

#include "pds/service/Client.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/ZcpFragment.hh"
#include <errno.h>
#include <string.h>
#include <stdio.h>

namespace Pds {

class BldClient: public Client 
{
public:
  BldClient(int sizeofDatagram, int maxPayload, const Ins& interface,
	        int maxDatagrams = 1, unsigned char ucTTL = 0):
    Client(sizeofDatagram,maxPayload,interface,maxDatagrams)
    {
      if(ucTTL != 0) {
        if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ucTTL, sizeof(ucTTL)) < 0)
          printf("*** Error: BldClient(): setsockopt- set TTL failed: %s\n", strerror(error()));
      }	  
    }

  ~BldClient() {}
};
  
};
#endif
