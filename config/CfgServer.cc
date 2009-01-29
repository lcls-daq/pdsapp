#include "CfgServer.hh"

#include "pds/config/CfgPorts.hh"
#include "pds/config/CfgRequest.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/utility/OutletWireHeader.hh"
#include "pds/utility/Mtu.hh"

#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

CfgServer::CfgServer(unsigned platform,
		     unsigned max_size,
		     unsigned nclients) :
  _platform(platform),
  _pool    (max_size,nclients),
  _max_size(max_size)
{
}

CfgServer::~CfgServer()
{
}

void CfgServer::run()
{
  // Listen for Requests
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  Ins ins(CfgPorts::ins(_platform));
  sockaddr_in saddr;
  saddr.sin_family      = AF_INET;
  saddr.sin_addr.s_addr = htonl(ins.address());
  saddr.sin_port        = htons(ins.portId());
  if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
  }

  char svcb[sizeof(CfgRequest)];
  CfgRequest& svc = *reinterpret_cast<CfgRequest*>(svcb);

  int nbytes;
  while(1) {
    socklen_t slen;
    nbytes  = ::recvfrom(sockfd, &svc, sizeof(svc), 0, (sockaddr*)&saddr, &slen);
    if (nbytes == sizeof(svc)) {
      void* p = _pool.alloc(_max_size);
      nbytes = fetch(p, svc);

      Datagram dg(svc.transition(),TypeId::Any,svc.src());
      dg.xtc.alloc(nbytes);
      OutletWireHeader header(&dg);
      iovec iov[2];
      msghdr msg;
      msg.msg_name    = &saddr;
      msg.msg_namelen = slen;
      msg.msg_iov     = iov;
      msg.msg_iovlen  = 2;
      iov[0].iov_base = &header;
      iov[0].iov_len  = sizeof(header);
      int remaining = header.length;
      while( remaining ) {
	iov[1].iov_base = (char*)p + header.offset;
	iov[1].iov_len  = (remaining+sizeof(header)) < Mtu::Size ?
	  remaining : Mtu::Size-sizeof(header);
	if ((nbytes=::sendmsg(sockfd, &msg, 0)) < 0) {
	  printf("CfgServer::sendmsg failed: %s\n",strerror(errno));
	  break;
	}
	remaining -= nbytes;
	header.offset += nbytes;
      }
      _pool.free(p);
    }
    else {
      printf("CfgServer received unknown request from %x/%d\n",
	     saddr.sin_addr.s_addr,saddr.sin_port);
    }
  }
}
