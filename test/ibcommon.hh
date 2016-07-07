#ifndef ibcommon_hh
#define ibcommon_hh

#include "pds/service/Ins.hh"
#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <vector>

#include <pthread.h>

//
//  Classes to implement an RDMA READ framework
//
namespace Pds {

  class IbReadReq {
  public:
    IbReadReq() {}
    IbReadReq(const char* p, unsigned s) : 
      addr(uintptr_t(p)), 
      len (s) {}
  public:
    uint64_t addr;
    uint32_t len;
  };

  class IbEndPt {
  public:
    uint32_t rkey;
    uint32_t qp_num;
    uint16_t lid;
    uint16_t reserved;
    uint8_t  gid[16];
  };

  class RdmaMasterCb { 
  public:
    virtual ~RdmaMasterCb() {}
  public:
    virtual void* request (const char* hdr, 
                           unsigned    payload_size) = 0;
    virtual void  complete(void*       payload) = 0;
  };

  class RdmaSlaveCb { 
  public:
    virtual ~RdmaSlaveCb() {}
  public:
    virtual void  complete(void*       payload) = 0;
  };

  class IbRdma {
  public:
    IbRdma(const char*   membase,
           unsigned      memsize,
           unsigned      hdrsize,
           const Ins&    remote);
    ~IbRdma();
  public:
    void             poll();
  private:
    virtual int      _handle_fd()=0;
    virtual int      _handle_cc()=0;
    virtual int      _handle_tmo() { return 1; }
  private:
    void             _setup(const char*, unsigned, const Ins&);
  protected:
    unsigned          _hdrsize;
    int               _fd;
    ibv_context*      _ctx;
    ibv_pd*           _pd;
    ibv_mr*           _mr;
    ibv_cq*           _cq;
    ibv_comp_channel* _cc;
    ibv_qp*           _qp;
    unsigned          _rkey;
    pthread_t         _thr;
  };

  class IbRdmaMaster : public IbRdma {
  public:
    IbRdmaMaster(const char*   membase,
                 unsigned      memsize,
                 unsigned      hdrsize,
                 const Ins&    remote,
                 RdmaMasterCb& cb);
    ~IbRdmaMaster();
  private:
    int      _handle_fd();
    int      _handle_cc();
    int      _handle_tmo();
  private:
    RdmaMasterCb&         _cb;
    char*                 _hdr;
    std::vector<uint64_t> _raddr;
    std::vector<void*   > _laddr;
    uint64_t              _wr_id;
    uint64_t              _wr_idc;
    uint64_t              _wr_ido;
  };

  class IbRdmaSlave : public IbRdma {
  public:
    IbRdmaSlave(const char*   membase,
                unsigned      memsize,
                unsigned      hdrsize,
                const Ins&    remote,
                RdmaSlaveCb&  cb);
    ~IbRdmaSlave();
  public:
    int req_read(const char* hdr,
                 const char* payload,
                 unsigned    payload_size);
  private:
    int      _handle_fd();
    int      _handle_cc();
  private:
    RdmaSlaveCb&  _cb;
  };
};

#endif
