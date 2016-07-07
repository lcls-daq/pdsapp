#include "pdsapp/test/ibcommon.hh"

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

using namespace Pds;

static const int ib_port=1;
static const unsigned MAX_WR=0x3f;

static void* rdmaReadThread(void* p);

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


IbRdma::IbRdma(const char*   membase,
               unsigned      memsize,
               unsigned      hdrsize,
               const Ins&    remote) :
  _hdrsize(hdrsize),
  _ctx(0),
  _pd (0),
  _mr (0),
  _cq (0),
  _qp (0)
{
  _setup(membase,memsize,remote);
}


IbRdma::~IbRdma()
{
  if (_qp) 
    ibv_destroy_qp(_qp);
  if (_mr)
    ibv_dereg_mr(_mr);
  if (_cq)
    ibv_destroy_cq(_cq);
//   if (_cc)
//     ibv_destroy_comp_channel(_cc);
  if (_pd)
    ibv_dealloc_pd(_pd);
  if (_ctx)
    ibv_close_device(_ctx);

  ::close(_fd);

  void* p;
  pthread_join(_thr, &p);
}

void IbRdma::_setup(const char* membase,
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
    return;
  }
  
//   if ((_cc = ibv_create_comp_channel(_ctx)) == 0) {
//     perror("Failed to create CC");
//     return;
//   }

  if ((_cq = ibv_create_cq(_ctx, 16, NULL, NULL, 0))==0) {
    perror("Failed to create CQ");
    return;
  }

  //  Register the memory buffers
  int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  void* p = reinterpret_cast<void*>(const_cast<char*>(membase));
  if ((_mr = ibv_reg_mr(_pd, p, memsize, mr_flags))==0) {
    perror("ibv_reg_mr failed");
    return;
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
  qp_init_attr.cap.max_send_wr = 16;
  qp_init_attr.cap.max_recv_wr = 1;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;

  if ((_qp = ibv_create_qp(_pd,&qp_init_attr))==0) {
    perror("Failed to create QP");
    return;
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
    
  printf("IbRdma setup with local base %p  len %u  lkey %x  rkey %x\n",
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
      return;
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
      return;
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
      return;
    }
  }

  pthread_attr_t tattr;
  pthread_attr_init(&tattr);
  if (pthread_create(&_thr, &tattr, &rdmaReadThread, this))
    perror("Error creating RDMA read thread");
}

void IbRdma::poll()
{
  pollfd pfd[2];
  pfd[0].fd     = _fd;
  pfd[0].events = POLLIN | POLLERR;
//   pfd[1].fd     = _cc->fd;
//   pfd[1].events = POLLIN | POLLERR;

  int rc;
  while((rc=::poll(pfd,1,100))>=0) {
    if (pfd[0].revents&POLLIN)
      if (!_handle_fd()) {
        printf("Remote end closed\n");
        break;
      }
//     if (pfd[1].revents&POLLIN)
//       _handle_cc();
    _handle_cc();
    if (rc==0)
      _handle_tmo();
  }
}


IbRdmaMaster::IbRdmaMaster(const char*   membase,
                           unsigned      memsize,
                           unsigned      hdrsize,
                           const Ins&    remote,
                           RdmaMasterCb& cb) :
  IbRdma(membase,memsize,hdrsize,remote),
  _cb   (cb),
  _hdr  (new char[hdrsize]),
  _raddr(MAX_WR+1),
  _laddr(MAX_WR+1),
  _wr_id(0),
  _wr_idc(0),
  _wr_ido(0)
{
}

IbRdmaMaster::~IbRdmaMaster()
{
}

int IbRdmaMaster::_handle_fd()
{
  IbReadReq req;
  iovec iov[2];
  iov[0].iov_base = &req;
  iov[0].iov_len  = sizeof(req);
  iov[1].iov_base = _hdr;
  iov[1].iov_len  = _hdrsize;

  int nb = ::readv(_fd, iov, 2);
  
  if (nb>0) {
    void* p = _cb.request(_hdr, req.len);
    //  launch dma
    {
      ibv_sge sge;
      memset(&sge, 0, sizeof(sge));
      sge.addr   = (uintptr_t)p;
      sge.length =  req.len;
      sge.lkey   = _mr->lkey;

      ibv_send_wr sr;
      memset(&sr,0,sizeof(sr));
      sr.next    = NULL;
      sr.wr_id   = _wr_id;
      sr.sg_list = &sge;
      sr.num_sge = 1;
      sr.opcode  = IBV_WR_RDMA_READ;
      sr.send_flags = IBV_SEND_SIGNALED;
      sr.wr.rdma.remote_addr = req.addr;
      sr.wr.rdma.rkey        = _rkey;

      _raddr[_wr_id&MAX_WR] = req.addr;
      _laddr[_wr_id&MAX_WR] = p;
      _wr_id++;

      ibv_send_wr* bad_wr=NULL;
      if (ibv_post_send(_qp, &sr, &bad_wr))
        perror("Failed to post SR");
      if (bad_wr)
        perror("ibv_post_send bad_wr");
    }
  }
  return nb;
}

int IbRdmaMaster::_handle_cc()
{
  // read from completion queue
  ibv_wc wc;
  while( ibv_poll_cq(_cq, 1, &wc)==1 ) {
    if (wc.opcode != IBV_WC_RDMA_READ)
      printf("handle_cc received opcode %d\n",wc.opcode);
    else if (wc.status != IBV_WC_SUCCESS)
      printf("handle_cc received status %d for id %lx\n",wc.status,wc.wr_id);
    else {
      if (wc.wr_id != _wr_idc)
        printf("Expected wr_id %lx found %lx\n", _wr_idc, wc.wr_id);
      _wr_idc++;
      //      printf("handle_cc[%lx] laddr %p raddr %lx\n", 
      //             wc.wr_id, _laddr[wc.wr_id], _raddr[wc.wr_id]);
      _cb.complete(_laddr[wc.wr_id&MAX_WR]);
      ::write(_fd, &_raddr[wc.wr_id&MAX_WR], sizeof(uint64_t));
    }
  }
  return 1;
}

int IbRdmaMaster::_handle_tmo()
{
  if (_wr_idc==_wr_ido && _wr_idc!=0) {  // No new completes since last tmo
    printf("--tmo--\n");
    ibv_port_attr a;
    if (ibv_query_port(_ctx, ib_port, &a))
      perror("ibv_query_port failed");
    else {
      dump_port(a);
    }
  }
  _wr_ido = _wr_idc;
}

IbRdmaSlave::IbRdmaSlave(const char*   base,
                         unsigned      size,
                         unsigned      hdrsize,
                         const Ins&    remote,
                         RdmaSlaveCb&  cb) :
  IbRdma(base,size,hdrsize,remote),
  _cb   (cb)
{
}

IbRdmaSlave::~IbRdmaSlave()
{
}

int IbRdmaSlave::_handle_fd()
{
  uint64_t paddr;
  int nb = ::read(_fd, &paddr, sizeof(paddr));

  if (nb>0)
    _cb.complete(reinterpret_cast<void*>(paddr));
  return nb;
}

int IbRdmaSlave::_handle_cc()
{
  // read from completion queue
  ibv_wc wc;
  while( ibv_poll_cq(_cq, 1, &wc) ) {
    printf("handle_cc received opcode %d\n",wc.opcode);
  }
  return 1;
}

int IbRdmaSlave::req_read(const char* hdr,
                          const char* payload,
                          unsigned    payload_size)
{
  IbReadReq req(payload, payload_size);
  iovec iov[2];
  iov[0].iov_base = &req;
  iov[0].iov_len  = sizeof(req);
  iov[1].iov_base = const_cast<char*>(hdr);
  iov[1].iov_len  = _hdrsize;
  return ::writev(_fd, iov, 2);
}

void* rdmaReadThread(void* p)
{
  IbRdma* rdma = reinterpret_cast<IbRdma*>(p);
  rdma->poll();
  return 0;
}


