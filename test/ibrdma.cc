#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include <infiniband/verbs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <inttypes.h>
#include <sys/time.h>

#define MAX_POLL_CQ_TIMEOUT 2000
// #define MSG      "SEND operation      "
// #define RDMAMSGR "RDMA read operation "
// #define RDMAMSGW "RDMA write operation"
// #define MSG_SIZE (strlen(MSG)+1)

#define MSG_SIZE 0x100000
static char MSG     [MSG_SIZE];
static char RDMAMSGR[MSG_SIZE];
static char RDMAMSGW[MSG_SIZE];

static inline uint64_t htonll(uint64_t x) { return __bswap_64(x); }
static inline uint64_t ntohll(uint64_t x) { return __bswap_64(x); }

struct config_t {
  char*    server_name;
  uint32_t tcp_port;
  int      ib_port;
  int      gid_idx;
};

struct cm_con_data_t {
  uint64_t addr;
  uint32_t rkey;
  uint32_t qp_num;
  uint16_t lid;
  uint8_t  gid[16];
} __attribute__((packed));

struct resources {
  ibv_device_attr device_attr;
  ibv_port_attr   port_attr;
  cm_con_data_t   remote_props;
  ibv_context*    ib_ctx;
  ibv_pd*         pd;
  ibv_cq*         cq;
  ibv_qp*         qp;
  ibv_mr*         mr;
  char*           buf;
  int             sock;
};

static config_t config = { NULL,
                           19877,
                           1,
                           -1 };

static int sock_connect(const char* servername, int port)
{
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (servername) {
    inet_aton(servername,&addr.sin_addr);
    if (connect(sockfd, (sockaddr*)&addr, sizeof(addr))) {
      perror("Failed to sonnect");
      close(sockfd);
      sockfd = -1;
    }
  }
  else {
    int fd = sockfd;
    sockfd = -1;
    addr.sin_addr.s_addr = 0;
    if (bind(fd, (sockaddr*)&addr, sizeof(addr))) {
      perror("Error binding");
    }
    else {
      listen(fd,1);
      sockfd = accept(fd,NULL,0);
    }
  }
  return sockfd;
}
  
static int sock_sync_data(int sock, int xfer_size, char* local_data, char* remote_data)
{
  int rc = write(sock, local_data, xfer_size);
  if (rc < xfer_size)
    perror("Failed writing data during sock_sync_data");
  else
    rc = 0;

  int read_bytes=0;
  int total_read_bytes=0;

  while(!rc && total_read_bytes < xfer_size) {
    read_bytes = read(sock, remote_data, xfer_size);
    if (read_bytes>0)
      total_read_bytes += read_bytes;
    else
      rc = read_bytes;
  }

  printf("sock_sync[%c]\n",*remote_data);

  return rc;
}

static int poll_completion(resources* res)
{
  int rc=0;
  timeval cur_time;
  gettimeofday(&cur_time,NULL);
  
  unsigned long start_time_msec = (cur_time.tv_sec*1000)+(cur_time.tv_usec/1000);

  int poll_result = 0;
  ibv_wc wc;
  unsigned long cur_time_msec=0;
  do {
    poll_result = ibv_poll_cq(res->cq,1,&wc);
    gettimeofday(&cur_time,NULL);
    cur_time_msec = (cur_time.tv_sec*1000)+(cur_time.tv_usec/1000);
  } while ((poll_result==0) && ((cur_time_msec-start_time_msec)<MAX_POLL_CQ_TIMEOUT));

  if (poll_result<0) {
    perror("poll CQ failed");
    rc=1;
  }
  else if (poll_result==0) {
    perror("Completion not found after TMO");
    rc=1;
  }
  else {
    printf("Completion found with status %x\n",wc.status);

    if (wc.status != IBV_WC_SUCCESS) {
      printf("Bad completion %x, %x\n", wc.status, wc.vendor_err);
      rc=1;
    }
  }
  return rc;
}

static int post_send(resources* res, int opcode)
{
  ibv_sge sge;
  memset(&sge, 0, sizeof(sge));
  sge.addr = (uintptr_t)res->buf;
  sge.length = MSG_SIZE;
  sge.lkey = res->mr->lkey;

  ibv_send_wr sr;
  memset(&sr,0,sizeof(sr));
  sr.next = NULL;
  sr.wr_id=0;
  sr.sg_list = &sge;
  sr.num_sge = 1;
  sr.opcode = (ibv_wr_opcode)opcode;
  sr.send_flags = IBV_SEND_SIGNALED;

  if (opcode != IBV_WR_SEND) {
    sr.wr.rdma.remote_addr = res->remote_props.addr;
    sr.wr.rdma.rkey = res->remote_props.rkey;
  }

  ibv_send_wr* bad_wr=NULL;
  int rc = ibv_post_send(res->qp, &sr, &bad_wr);
  if (rc)
    perror("Failed to post SR");
  else {
    switch(opcode) {
    case IBV_WR_SEND:
      printf("Send request was posted\n");
      break;
    case IBV_WR_RDMA_READ:
      printf("RDMA Read Request was posted\n");
      break;
    case IBV_WR_RDMA_WRITE:
      printf("RDMA Write Request was posted\n");
      break;
    default:
      printf("Unknown request was posted\n");
      break;
    }
  }
  return rc;
}

static int post_receive(resources* res)
{
  ibv_sge sge;
  memset(&sge, 0, sizeof(sge));
  sge.addr = (uintptr_t)res->buf;
  sge.length = MSG_SIZE;
  sge.lkey = res->mr->lkey;

  ibv_recv_wr rr;
  memset(&rr,0,sizeof(rr));
  rr.next = NULL;
  rr.wr_id = 0;
  rr.sg_list = &sge;
  rr.num_sge = 1;

  ibv_recv_wr* bad_wr;
  int rc = ibv_post_recv(res->qp, &rr, &bad_wr);
  if (rc)
    perror("Failed to post RR");
  else
    printf("Receive Request was posted\n");

  return rc;
}

static void resources_init(resources* res)
{
  memset(res, 0, sizeof(*res));
  res->sock = -1;
}

static int resources_create(resources* res)
{
  int rc = -1;

  ibv_device** dev_list = NULL;

  do {
    if (config.server_name) {
      if ((res->sock = sock_connect(config.server_name,
                                    config.tcp_port)) < 0) {
        perror("Failed to establish TCP connection");
        break;
      }
    }
    else {
      printf("Waiting on port %d for TCP connect\n",
             config.tcp_port);
      if ((res->sock = sock_connect(NULL,
                                    config.tcp_port))<0) {
        perror("Failed to establish TCP connection");
        break;
      }
    }
    printf("TCP connect established\n");
    rc = 1;

    int num_devices=0;
    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list) {
      perror("Failed to get IB devices list\n");
      break;
    }

    printf("Found %d devices\n", num_devices);
    
    if (!num_devices) {
      break;
    }

    ibv_device* ib_dev = dev_list[0];
    if ((res->ib_ctx = ibv_open_device(ib_dev))==0) {
      perror("Failed to open ib device");
      break;
    }

    ibv_free_device_list(dev_list);
    dev_list = NULL;
    ib_dev = NULL;

    //  Query port properties
    if (ibv_query_port(res->ib_ctx, config.ib_port, &res->port_attr)) {
      perror("ibv_query_port failed");
      break;
    }

    //  Allocate Protection Domain
    if ((res->pd = ibv_alloc_pd(res->ib_ctx))==0) {
      perror("ibv_alloc_pd failed");
      break;
    }

    if ((res->cq = ibv_create_cq(res->ib_ctx, 1, NULL, NULL, 0))==0) {
      perror("Failed to create CQ");
      break;
    }

    if ((res->buf = (char*)malloc(MSG_SIZE))==0) {
      perror("Failed to malloc buffer");
      break;
    }

    //    memset(res->buf,0,MSG_SIZE);

    if (!config.server_name) {
      memcpy(res->buf,MSG,MSG_SIZE);
      printf("Send the message is [%s][%s]\n",res->buf,res->buf+MSG_SIZE-32);
    }

    //  Register the memory buffer
    int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    if ((res->mr = ibv_reg_mr(res->pd, res->buf, MSG_SIZE, mr_flags))==0) {
      perror("ibv_reg_mr failed");
      break;
    }

    printf("MR[%zu] was registered with addr %p, lkey %x, rkey %x, flags %x\n",
           sizeof(*res->mr), res->buf, res->mr->lkey, res->mr->rkey, mr_flags);

    //  Create the QP
    ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr,0,sizeof(qp_init_attr));

    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.sq_sig_all = 1;
    qp_init_attr.send_cq = res->cq;
    qp_init_attr.recv_cq = res->cq;
    qp_init_attr.cap.max_send_wr = 1;
    qp_init_attr.cap.max_recv_wr = 1;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;

    if ((res->qp = ibv_create_qp(res->pd,&qp_init_attr))==0) {
      perror("Failed to create QP");
      break;
    }

    rc = 0;
    printf("QP was create, QP number %x\n",
           res->qp->qp_num);
  } while(0);

  if (dev_list)
    ibv_free_device_list(dev_list);
    
  return rc;
}

static int resources_destroy(resources* res)
{
  //  Cleanup
  if (res->qp) 
    ibv_destroy_qp(res->qp);
  if (res->mr)
    ibv_dereg_mr(res->mr);
  if (res->buf) {
    free(res->buf);
    res->buf=NULL;
  }
  if (res->cq)
    ibv_destroy_cq(res->cq);
  if (res->pd)
    ibv_dealloc_pd(res->pd);
  if (res->ib_ctx)
    ibv_close_device(res->ib_ctx);
  if (res->sock >= 0) 
    close(res->sock);

  return 0;
}

static int modify_qp_to_init(ibv_qp* qp)
{
  ibv_qp_attr attr;

  memset(&attr,0,sizeof(attr));
  attr.qp_state = IBV_QPS_INIT;
  attr.port_num = config.ib_port;
  attr.pkey_index = 0;
  attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | 
    IBV_ACCESS_REMOTE_READ |
    IBV_ACCESS_REMOTE_WRITE;

  int flags = IBV_QP_STATE|IBV_QP_PKEY_INDEX|IBV_QP_PORT|IBV_QP_ACCESS_FLAGS;

  int rc = ibv_modify_qp(qp,&attr,flags);
  if (rc)
    perror("Failed to modify QT state to INIT");

  return rc;
}

static int modify_qp_to_rtr(ibv_qp* qp, uint32_t remote_qpn, uint16_t dlid, uint8_t* dgid)
{
  ibv_qp_attr attr;
  memset(&attr,0,sizeof(attr));

  attr.qp_state = IBV_QPS_RTR;
  attr.path_mtu = IBV_MTU_256;
  attr.dest_qp_num = remote_qpn;
  attr.rq_psn = 0;
  attr.max_dest_rd_atomic=1;
  attr.min_rnr_timer=0x12;
  attr.ah_attr.is_global=0;
  attr.ah_attr.dlid=dlid;
  attr.ah_attr.sl=0;
  attr.ah_attr.src_path_bits=0;
  attr.ah_attr.port_num = config.ib_port;

  int flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
    IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
    IBV_QP_MIN_RNR_TIMER;

  int rc = ibv_modify_qp(qp,&attr,flags);
  if (rc)
    perror("Failed to modify QP state to RTR");

  return rc;
}

static int modify_qp_to_rts(ibv_qp* qp)
{
  ibv_qp_attr attr;

  memset(&attr,0,sizeof(attr));
  attr.qp_state = IBV_QPS_RTS;
  attr.timeout = 0x12;
  attr.retry_cnt = 6;
  attr.rnr_retry=0;
  attr.sq_psn = 0;
  attr.max_rd_atomic=1;

  int flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
    IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

  int rc = ibv_modify_qp(qp,&attr,flags);
  if (rc)
    perror("Failed to modify QP state to RTS");

  return rc;
}

static int connect_qp(resources* res)
{
  int rc=0;
  cm_con_data_t local_con_data;
  cm_con_data_t remote_con_data;
  cm_con_data_t tmp_con_data;

  ibv_gid my_gid;

  if (config.gid_idx>=0) {
    rc = ibv_query_gid(res->ib_ctx, config.ib_port, config.gid_idx, &my_gid);
    if (rc) {
      printf("Could not get tid for port %d, index %d\n",
             config.ib_port, config.gid_idx);
      return rc;
    }
  }
  else 
    memset(&my_gid,0,sizeof(my_gid));

  local_con_data.addr   = htonll((uintptr_t)res->buf);
  local_con_data.rkey   = htonl (res->mr->rkey);
  local_con_data.qp_num = htonl (res->qp->qp_num);
  local_con_data.lid    = htons (res->port_attr.lid);
  memcpy(local_con_data.gid, &my_gid, 16);
  
  printf("Local LID %x\n",res->port_attr.lid);

  do {
    rc=1;
    if (sock_sync_data(res->sock, sizeof(cm_con_data_t), (char*)&local_con_data,
                       (char*)&tmp_con_data) < 0) {
      perror("Failed to exchange con_data");
      break;
    }

    remote_con_data.addr   = ntohll(tmp_con_data.addr);
    remote_con_data.rkey   = ntohl (tmp_con_data.rkey);
    remote_con_data.qp_num = ntohl (tmp_con_data.qp_num);
    remote_con_data.lid    = ntohs (tmp_con_data.lid);
    memcpy(remote_con_data.gid, tmp_con_data.gid, 16);

    res->remote_props = remote_con_data;

    printf("Remote addr %lx\n",remote_con_data.addr);
    printf("Remote rkey %x\n", remote_con_data.rkey);
    printf("Remote QP number %x\n", remote_con_data.qp_num);
    printf("Remote LID %x\n", remote_con_data.lid);

    if (config.gid_idx >= 0) {
      uint8_t* p = remote_con_data.gid;
      printf("Remote GID = ");
      for(unsigned i=0; i<16; i++)
        printf("%02x%c",p[i],i<15?':':'\n');
    }

    if ((rc = modify_qp_to_init(res->qp))) {
      perror("Change QP state to INIT failed");
      break;
    }

    if (config.server_name) {
      if ((rc = post_receive(res))) {
        perror("Failed to post RR");
        break;
      }
    }

    if ((rc=modify_qp_to_rtr(res->qp, remote_con_data.qp_num, 
                             remote_con_data.lid, remote_con_data.gid))) {
      perror("Failed to modify QP state to RTR");
      break;
    }
    
    if ((rc=modify_qp_to_rts(res->qp))) {
      perror("Failed to modify QP state to RTS");
      break;
    }

    printf("QP state is RTS\n");

    char temp_char;
    if (sock_sync_data(res->sock, 1, "Q", &temp_char)) {
      perror("sync error after QPs moved to RTS");
      rc=1;
      break;
    }
  } while(0);
  
  return rc;
}

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

  resources res;

  resources_init(&res);

  int rc=0;
  char temp_char;
  do {
    if (resources_create(&res)) {
      perror("Creating resources");
      break;
    }

    if (connect_qp(&res)) {
      perror("Failed to connect QPs");
      break;
    }

    if (!config.server_name)
      if (post_send(&res, IBV_WR_SEND)) {
        perror("Failed to post sr");
        break;
      }

    if (poll_completion(&res)) {
      perror("Poll completion failed");
      break;
    }

    if (config.server_name) {
      printf("Message is [%s][%s]\n",res.buf,res.buf+MSG_SIZE-32);
    }
    else {
      memcpy(res.buf,RDMAMSGR,MSG_SIZE);
    }
    
    rc=1;
    if (sock_sync_data(res.sock,1,"R",&temp_char)) {
      perror("Sync error before RMDA ops");
      break;
    }

    if (config.server_name) {
      if (post_send(&res, IBV_WR_RDMA_READ)) {
        perror("Failed to post SR 2");
        break;
      }

      if (poll_completion(&res)) {
        perror("Poll completion failed");
        break;
      }

      printf("Contents of server buffer [%s][%s]\n",res.buf,res.buf+MSG_SIZE-32);
      memcpy(res.buf,RDMAMSGW,MSG_SIZE);

      printf("Now replacing with [%s][%s]\n",res.buf,res.buf+MSG_SIZE-32);

      if (post_send(&res,IBV_WR_RDMA_WRITE)) {
        perror("Failed to post SR 3");
        break;
      }

      if (poll_completion(&res)) {
        perror("Poll completion failed 3");
        break;
      }
    }

    if (sock_sync_data(res.sock,1,"W",&temp_char)) {
      perror("Sync error after RDMA ops");
      break;
    }

    if (!config.server_name)
      printf("Contents of server buffer [%s][%s]\n",res.buf,res.buf+MSG_SIZE-32);

    rc=0;

  } while(0);

  if (resources_destroy(&res))
    perror("Failed to destroy resources");

  return rc;
}
