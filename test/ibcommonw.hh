#ifndef ibcommonw_hh
#define ibcommonw_hh

#include "pds/service/Ins.hh"
#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <vector>

#include <pthread.h>

//
//  Classes to implement an RDMA WRITE framework
//
namespace Pds {

  namespace IbW {

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
      virtual void  complete(void*       payload) = 0;
    };

    class RdmaSlaveCb { 
    public:
      virtual ~RdmaSlaveCb() {}
    public:
      virtual void  complete(void*       payload) = 0;
    };

    class Rdma {
    public:
      Rdma(const char*   membase,
           unsigned      memsize,
           const Ins&    remote);
      ~Rdma();
    public:
      void             poll();
    private:
      virtual int      _handle_cc()=0;
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

    class RdmaMaster : public Rdma {
    public:
      RdmaMaster(const char*   membase,
                 unsigned      memsize,
                 const Ins&    remote,
                 RdmaMasterCb& cb);
      ~RdmaMaster();
    public:
      void     req_write(void*    payload,
                         unsigned size,
                         unsigned index);
    public:
      void     poll_complete();
    private:
      int      _handle_cc();
    private:
      RdmaMasterCb&         _cb;
      char*                 _hdr;
      std::vector<uint64_t> _raddr;
      std::vector<void*   > _laddr;
      uint32_t              _wr_id;
      uint32_t              _wr_idc;
    };

    class RdmaSlave : public Rdma {
    public:
      RdmaSlave(const char*               membase,
                unsigned                  memsize,
                const std::vector<char*>& memv,
                const Ins&                remote,
                RdmaSlaveCb&              cb);
      ~RdmaSlave();
    private:
      int      _handle_cc();
      void     _post_wr  (unsigned);
    private:
      RdmaSlaveCb&             _cb;
      std::vector<char*>       _laddr;
      std::vector<uint32_t*>   _rimm;
      std::vector<ibv_recv_wr> _wr;
    };
  };
};

#endif
