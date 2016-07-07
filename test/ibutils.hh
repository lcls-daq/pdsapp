#ifndef ibutils_hh
#define ibutils_hh

namespace Pds {
  namespace Rdma {

    class Target {
    public:
      uint64_t remote_addr;
      uint32_t rkey;
    };

    //
    //  Only consider the RDMA_WRITE operation
    //    Servers write data to their clients
    //
    class Server {
    public:
      Server(unsigned cq_depth);
      ~Server();
    public:
      //  Register a buffer with the server
      //     Adds to protection domain
      //     Returns index of buffer
      int   register(char* buffer, unsigned sz);

      //  Separate completion thread for recoving buffers
      //  Return to application or driver
      //  void  ready   (int buffer);

      //  Send the buffer contents to the client (client's nth buffer)
      //     We don't need to know if the client/target is ready
      int   send    (int buffer, unsigned sz, const Target&);
    private:
      ibv_context* _ctx; // IB context
      ibv_pd*      _pd;  // IB protection domain
      ibv_cq*      _cq;  // IB completion queue
      ibv_qp*      _qp;  // IB queue pair
    };

    //
    //  Manage connections from clients to a server
    //    Listen to a multicast group where clients discover servers
    //      followed by a TCP connection to exchange queue pair information
    //
    class ConnectionManager {
    public:
      ConnectionManager(Server&, const Pds::Src&, Pds::Ins mcast);
    public:
      const Target* target(unsigned key) const;
    private:
      Server&             _server;
      int                 _fd;
      std::vector<Target> _targets;
    };

    class ConnectionRequestor {
    public:
      ConnectionRequestor(Pds::Ins mcast);
    public:
      std::list<Pds::Src> sources();
    public:
      void connect(const Pds::Src&, const std::vector<Target>&);
    private:
      int _discovery_fd;  // UDP socket
      int _exchange_fd;   // TCP socket
    };
  };
};

#endif
