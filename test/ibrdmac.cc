#include "pdsapp/test/ibcommon.hh"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

namespace Pds {
  class MasterCb : public RdmaMasterCb {
  public:
    MasterCb() {}
  public:
    virtual void* request (const char* hdr, 
                           unsigned    payload_size) { return 0; }
    virtual void  complete(void*       payload) {}
  };

  class SlaveCb : public RdmaSlaveCb {
  public:
    SlaveCb() {}
  public:
    virtual void  complete(void*       payload) {}
  };
};

using namespace Pds;

#define MAX_POLL_CQ_TIMEOUT 2000
// #define MSG      "SEND operation      "
// #define RDMAMSGR "RDMA read operation "
// #define RDMAMSGW "RDMA write operation"
// #define MSG_SIZE (strlen(MSG)+1)

#define MSG_SIZE 0x100000
static char MSG     [MSG_SIZE];
static char RDMAMSGR[MSG_SIZE];
static char RDMAMSGW[MSG_SIZE];

struct config_t {
  char*    server_name;
  uint32_t tcp_port;
  int      ib_port;
  int      gid_idx;
};

static config_t config = { NULL,
                           19877,
                           1,
                           -1 };

int main(int argc, char* argv[])
{
  int c;
  while((c=getopt(argc,argv,"p:i:g:"))!=-1) {
    switch(c) {
    case 'p':
      config.tcp_port = strtoul(optarg,NULL,0);
      break;
    case 'i':
      config.ib_port = strtoul(optarg,NULL,0);
      break;
    case 'g':
      config.gid_idx = strtoul(optarg,NULL,0);
      break;
    default:
      break;
    }
  }
  if (optind == argc-1)
    config.server_name = argv[optind];


  sprintf(MSG     ,"SEND operation      ");
  sprintf(RDMAMSGR,"RDMA read operation ");
  sprintf(RDMAMSGW,"RDMA write operation");

  sprintf(MSG     +MSG_SIZE-32,"done SEND");
  sprintf(RDMAMSGR+MSG_SIZE-32,"done read");
  sprintf(RDMAMSGW+MSG_SIZE-32,"done writ");

  if (ibv_fork_init())
    perror("ibv_fork_init failed");

  //  char* buf = new char[MSG_SIZE];
  char* buf = (char*)malloc(MSG_SIZE);

  IbRdma* rdma;
  if (config.server_name) {
    in_addr ia;
    inet_aton(config.server_name,&ia);
    rdma = new IbRdmaSlave(buf, MSG_SIZE, 32, 
                           Ins(ntohl(ia.s_addr),
                              (unsigned short)config.tcp_port),
                           *new SlaveCb);
  }
  else
    rdma = new IbRdmaMaster(buf, MSG_SIZE, 32, Ins((unsigned short)config.tcp_port),
                            *new MasterCb);

  while(1) {
    sleep(1);
  }
  return 1;
}
   
