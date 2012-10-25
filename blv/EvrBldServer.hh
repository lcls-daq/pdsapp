#ifndef PDS_EvrBldServer_hh
#define PDS_EvrBldServer_hh

#include "pds/utility/EvrServer.hh"

#include "pds/mon/THist.hh"

namespace Pds {

  class EvrBldServer : public EvrServer {
  public:
    EvrBldServer(const Src& client, int read_fd, InletWire& inlet);
    ~EvrBldServer() {}    
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const Xtc&  xtc   () const;
    bool        more  () const;
    unsigned    offset() const;
    unsigned    length() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend  (int flag = 0);
    int      fetch (char* payload, int flags);
    int      fetch (ZcpFragment& , int flags);
  public:
    const Sequence&     sequence() const;
    const L1AcceptEnv&  env() const;
    unsigned            count() const;
  private:
    Xtc          _xtc;
    unsigned     _count;
    EvrDatagram* _evrDatagram;   	
    timespec     _tfetch;
    THist        _hinput;
    THist        _hfetch;
  };
}

inline const Pds::Sequence& Pds::EvrBldServer::sequence() const
{
  const Pds::EvrDatagram& dg = *_evrDatagram;
  return dg.seq;
}

inline const Pds::L1AcceptEnv& Pds::EvrBldServer::env() const
{
  const Pds::EvrDatagram& dg = *_evrDatagram;
  return dg.env;
}

inline unsigned Pds::EvrBldServer::count() const
{
  const Pds::EvrDatagram& dg = *_evrDatagram;
  return dg.evr;
}

#endif


