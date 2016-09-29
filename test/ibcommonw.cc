#include "pdsapp/test/ibcommonw.hh"

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

using namespace Pds::IbW;

static const int ib_port=1;
static const unsigned MAX_WR=0x3f;

static void* rdmaReadThread(void* p);
static void* rdmaCompleteThread(void* p);

enum { ReadRequest, ReadDone };

#define P_U(v) { printf("%s: 0x%x\n",#v,a.v); }

static void dump_port(ibv_port_attr& a)
{
  P_U(state);
  P_U(max_mtu);
  P_U(active_mtu);
  P_U(gid_tbl_len);
  P_U(port_cap_flags);
  P_U(max_msg_sz);
  P_U(bad_pkey_cntr);
  P_U(qkey_viol_cntr);
  P_U(pkey_tbl_len);
  P_U(lid);
  P_U(sm_lid);
  P_U(lmc);
  P_U(max_vl_num);
  P_U(sm_sl);
  P_U(subnet_timeout);
  P_U(init_type_reply);
  P_U(active_width);
  P_U(active_speed);
  P_U(phys_state);
  P_U(link_layer);
}


Rdma::Rdma(const char*   membase,
           unsigned      memsize,
           const Ins&    remote) :
  _ctx(0),
  _pd (0),
  _mr (0),
  _cq (0),
  _cc (0),
  _qp (0)
{
  _setup(membase,memsize,remote);
}


Rdma::~Rdma()
{
  if (_qp) 
    ibv_destroy_qp(_qp);
  if (_mr)
    ibv_dereg_mr(_mr);
  if (_cq)
    ibv_destroy_cq(_cq);
  if (_cc)
    ibv_destroy_comp_channel(_cc);
  if (_pd)
    ibv_dealloc_pd(_pd);
  if (_ctx)
    ibv_close_device(_ctx);

  ::close(_fd);

  void* p;
  pthread_join(_thr, &p);
}

void Rdma::_setup(const char* membase,
                    unsigned    memsize,
                    const Ins&  remote)
{
  printf("Open socket for %x.%d\n",remote.address(),remote.portId());

  sockaddr_in rsaddr;
  rsaddr.sin_family = AF_INET;
  rsaddr.sin_addr.s_addr = htonl(remote.address());
  rsaddr.sin_port        = htons(remote.portId ());

  if (remote.address()==INADDR_ANY) {
    int lfd = ::socket(AF_INET, SOCK_STREAM, 0);
    int oval=1;
    ::setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &oval, sizeof(oval));
    ::bind(lfd, (sockaddr*)&rsaddr, sizeof(rsaddr));
    ::listen(lfd, 5);
    { sockaddr_in saddr;
      socklen_t   saddr_len=sizeof(saddr);
      _fd = ::accept(lfd, (sockaddr*)&saddr, &saddr_len); }
    close(lfd);
  }
  else {
    _fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (::connect(_fd, (sockaddr*)&rsaddr, sizeof(rsaddr))<0) {
      perror("Failed to connect");
      return;
    }
  }

  {
    int num_devices=0;
    ibv_device** dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list) {
      printf("Failed to get IB devices list\n");
      return;
    }
  
    printf("Found %d devices\n", num_devices);
    if (!num_devices)
      return;
    
    _ctx = ibv_open_device(dev_list[0]);
    ibv_free_device_list(dev_list);
  }

  //  Query port properties
  ibv_port_attr   port_attr;
  if (ibv_query_port(_ctx, ib_port, &port_attr)) {
    perror("ibv_query_port failed");
    return;
  }
  
  //  Allocate Protection Domain
  if ((_pd = ibv_alloc_pd(_ctx))==0) {
    perror("ibv_alloc_pd failed");
    abort();
  }
  
  if ((_cc = ibv_create_comp_channel(_ctx)) == 0) {
    perror("Failed to create CC");
    abort();
  }

  if ((_cq = ibv_create_cq(_ctx, 16, NULL, _cc, 0))==0) {
    perror("Failed to create CQ");
    abort();
  }

  //  Register the memory buffers
  int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  void* p = reinterpret_cast<void*>(const_cast<char*>(membase));
  if ((_mr = ibv_reg_mr(_pd, p, memsize, mr_flags))==0) {
    perror("ibv_reg_mr failed");
    abort();
  }

  printf("MR[%zu] was registered with addr %p, lkey %x, rkey %x, flags %x\n",
         sizeof(*_mr), p, _mr->lkey, _mr->rkey, mr_flags);

  //  Create the QP
  ibv_qp_init_attr qp_init_attr;
  memset(&qp_init_attr,0,sizeof(qp_init_attr));

  qp_init_attr.qp_type = IBV_QPT_RC;
  qp_init_attr.sq_sig_all = 1;
  qp_init_attr.send_cq = _cq;
  qp_init_attr.recv_cq = _cq;
  qp_init_attr.cap.max_send_wr = 32;
  qp_init_attr.cap.max_recv_wr = 32;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;
  qp_init_attr.cap.max_inline_data = 32*sizeof(unsigned);

  if ((_qp = ibv_create_qp(_pd,&qp_init_attr))==0) {
    perror("Failed to create QP");
    abort();
  }

  printf("QP was created, QP number %x\n",
         _qp->qp_num);

  ibv_gid gid;
  //  ibv_query_gid(_ctx, ib_port, 0, &gid);
  memset(&gid,0,sizeof(gid));

  IbEndPt endpt;
  endpt.rkey   = _mr->rkey;
  endpt.qp_num = _qp->qp_num;
  endpt.lid    = port_attr.lid;
  memcpy(&endpt.gid, &gid, 16);

  printf("Local LID %x\n",port_attr.lid);

  ::write(_fd, &endpt, sizeof(endpt));
  { int bytes=0, remaining=sizeof(endpt);
    char* p = reinterpret_cast<char*>(&endpt);
    while(remaining) {
      bytes = ::read(_fd, p, remaining);
      remaining -= bytes;
      p += bytes;
    }
  }

  //  printf("Remote addr %lx\n",endpt.addr);
  printf("Remote rkey %x\n", endpt.rkey);
  printf("Remote QP number %x\n", endpt.qp_num);
  printf("Remote LID %x\n", endpt.lid);
    
  printf("Rdma setup with local base %p  len %u  lkey %x  rkey %x\n",
         membase,memsize,_mr->rkey,endpt.rkey);

  _rkey = endpt.rkey;

  {
    ibv_qp_attr attr;

    memset(&attr,0,sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = ib_port;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | 
      IBV_ACCESS_REMOTE_READ |
      IBV_ACCESS_REMOTE_WRITE;

    int flags = IBV_QP_STATE|IBV_QP_PKEY_INDEX|IBV_QP_PORT|IBV_QP_ACCESS_FLAGS;

    int rc = ibv_modify_qp(_qp,&attr,flags);
    if (rc) {
      perror("Failed to modify QT state to INIT");
      abort();
    }
  }

  {
    ibv_qp_attr attr;
    memset(&attr,0,sizeof(attr));
    
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_256;
    attr.dest_qp_num = endpt.qp_num;
    attr.rq_psn      = 0;
    attr.max_dest_rd_atomic=1;
    attr.min_rnr_timer=0x12;
    attr.ah_attr.is_global=0;
    attr.ah_attr.dlid= endpt.lid;
    attr.ah_attr.sl  = 0;
    attr.ah_attr.src_path_bits=0;
    attr.ah_attr.port_num = ib_port;
    
    int flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
      IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
      IBV_QP_MIN_RNR_TIMER;
    
    int rc = ibv_modify_qp(_qp,&attr,flags);
    if (rc) {
      perror("Failed to modify QP state to RTR");
      abort();
    }
  }
 
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
    
    int rc = ibv_modify_qp(_qp,&attr,flags);
    if (rc) {
      perror("Failed to modify QP state to RTS");
      abort();
    }
  }

  pthread_attr_t tattr;
  pthread_attr_init(&tattr);
  if (pthread_create(&_thr, &tattr, &rdmaReadThread, this))
    perror("Error creating RDMA read thread");
}

void Rdma::poll()
{
  void*   cq_ctx=0;
  ibv_cq* cq    =0;
  ibv_req_notify_cq(_cq,0);
  while( ibv_get_cq_event(_cc,&cq,&cq_ctx)>=0 ) {  // blocking operation
    if (cq == _cq) {
      ibv_ack_cq_events(_cq,1);
      ibv_req_notify_cq(_cq,0);
      _handle_cc();
    }
    else
      printf("Unexpected compl_queue %p\n",cq);
  }
}


RdmaMaster::RdmaMaster(const char*   membase,
                       unsigned      memsize,
                       const Ins&    remote,
                       RdmaMasterCb& cb) :
  Rdma(membase,memsize,remote),
  _cb   (cb),
  _wr_id(0),
  _wr_idc(0)
{
  unsigned max_wr;
  ::read(_fd, &max_wr, sizeof(unsigned));
  _raddr.resize(max_wr);
  _laddr.resize(max_wr);
  ::read(_fd, _raddr.data(), max_wr*sizeof(void*));

  pthread_attr_t tattr;
  pthread_attr_init(&tattr);
  pthread_t      thr;
  if (pthread_create(&thr, &tattr, &rdmaCompleteThread, this))
    perror("Error creating RDMA read thread");
}

RdmaMaster::~RdmaMaster()
{
}

void RdmaMaster::req_write(void*    p, 
                           unsigned size, 
                           unsigned index)
{
  //  launch dma
  ibv_sge sge;
  memset(&sge, 0, sizeof(sge));
  sge.addr   = (uintptr_t)p;
  sge.length =  size;
  sge.lkey   = _mr->lkey;

  ibv_send_wr sr;
  memset(&sr,0,sizeof(sr));
  sr.next    = NULL;
  // stuff index into upper 32 bits
  sr.wr_id   = (uint64_t(index)<<32) | _wr_id;
  sr.sg_list = &sge;
  sr.num_sge = 1;
  sr.imm_data= index;
  sr.opcode  = IBV_WR_RDMA_WRITE_WITH_IMM;
  sr.send_flags = IBV_SEND_SIGNALED;
  sr.wr.rdma.remote_addr = _raddr[index];
  sr.wr.rdma.rkey        = _rkey;

  _laddr[index] = (char*)p;
  _wr_id++;

  ibv_send_wr* bad_wr=NULL;
  if (ibv_post_send(_qp, &sr, &bad_wr))
    perror("Failed to post SR");
  if (bad_wr)
    perror("ibv_post_send bad_wr");
}

int RdmaMaster::_handle_cc()
{
  // read from completion queue
  ibv_wc wc;
  while( ibv_poll_cq(_cq, 1, &wc)==1 ) {
    if (wc.opcode != IBV_WC_RDMA_WRITE)
      printf("handle_cc received opcode %d\n",wc.opcode);
    else if (wc.status != IBV_WC_SUCCESS)
      printf("handle_cc received status %d for id %lx\n",wc.status,wc.wr_id);
    else {
      unsigned wr_id = wc.wr_id&0xffffffff;
      if (wr_id != _wr_idc)
        printf("Expected wr_id %x found %x\n", _wr_idc, wr_id);
      _wr_idc++;
      //      printf("handle_cc[%lx] laddr %p raddr %lx\n", 
      //             wc.wr_id, _laddr[wc.wr_id], _raddr[wc.wr_id]);
      //      _cb.complete(_laddr[wc.wr_id>>32]);
    }
  }
  return 1;
}

RdmaSlave::RdmaSlave(const char*   base,
                     unsigned      size,
                     const std::vector<char*>& pool,
                     const Ins&    remote,
                     RdmaSlaveCb&  cb) :
  Rdma  (base,size,remote),
  _cb   (cb),
  _laddr(pool),
  _rimm (pool.size()),
  _wr   (pool.size()),
  _nc   (0)
{
  //  Send indexed list of target addresses to master
  unsigned nmem = pool.size();
  iovec iov[2];
  iov[0].iov_base = &nmem;
  iov[0].iov_len  = sizeof(unsigned);
  iov[1].iov_base = _laddr.data();
  iov[1].iov_len  = _laddr.size()*sizeof(void*);
  ::writev(_fd,iov,2);

  for(unsigned i=0; i<_wr.size(); i++) {
    ibv_sge &sge = *new ibv_sge;
    memset(&sge, 0, sizeof(sge));
    sge.addr   = (uintptr_t)new unsigned;
    sge.length =  sizeof(unsigned);
    sge.lkey   = _mr->lkey;
    
    ibv_recv_wr &sr = _wr[i];
    memset(&sr,0,sizeof(sr));
    sr.next    = NULL;
    // stuff index into upper 32 bits
    sr.sg_list = &sge;
    sr.num_sge = 1;
    sr.wr_id   = i;

    ibv_recv_wr* bad_wr=NULL;
    if (ibv_post_recv(_qp, &sr, &bad_wr))
      perror("Failed to post SR");
    if (bad_wr)
      perror("ibv_post_recv bad_wr");
  }    
}

RdmaSlave::~RdmaSlave()
{
}

int RdmaSlave::_handle_cc()
{
  // read from completion queue
  ibv_wc wc;
  while( ibv_poll_cq(_cq, 1, &wc) ) {
    if (wc.opcode != IBV_WC_RECV_RDMA_WITH_IMM)
      printf("handle_cc received opcode %d\n",wc.opcode);
    else if (wc.status != IBV_WC_SUCCESS)
      printf("handle_cc received status %d for id %lx\n",wc.status,wc.wr_id);
    else {
//       printf("handle_cc[%lx] nbytes %u  imm %x\n",
//              wc.wr_id, wc.byte_len, wc.imm_data);
      _cb.complete(_laddr[wc.imm_data]);
      
      ibv_recv_wr* bad_wr=NULL;
      if (ibv_post_recv(_qp, &_wr[wc.imm_data], &bad_wr))
        perror("Failed to post SR");

      if (bad_wr)
        perror("ibv_post_recv bad_wr");

      ::write(_fd, &wc.imm_data, sizeof(unsigned));
    }
  }
  _nc++;
  return 1;
}

void RdmaSlave::dump() const { printf("%u callbacks\n",_nc); }

void* rdmaReadThread(void* p)
{
  Rdma* rdma = reinterpret_cast<Rdma*>(p);
  rdma->poll();
  return 0;
}


void* rdmaCompleteThread(void* p)
{
  RdmaMaster* rdma = reinterpret_cast<RdmaMaster*>(p);
  rdma->poll_complete();
  return 0;
}

void RdmaMaster::poll_complete()
{
  while(1) {
    unsigned index;
    int nb = ::read(_fd, &index, sizeof(index));
    if (nb<0) {
      printf("Remote end closed\n");
      break;
    }
    if (nb == sizeof(index))
      _cb.complete(_laddr[index]);
  }
}
