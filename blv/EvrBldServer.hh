#ifndef PDS_EvrBldServer_hh
#define PDS_EvrBldServer_hh

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/EvrDatagram.hh"

namespace Pds {

  class CDatagram;
  class ZcpDatagram;

  class EvrBldServer : public EbServer, public EbCountSrv {
  public:
    EvrBldServer(const Src& client);
    ~EvrBldServer() {}    
    int  sendEvrEvent(EvrDatagram* evrDatagram);
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
    unsigned count() const;
  private:
    int        _pipefd[2];
    Xtc        _xtc;
    unsigned   _count;
    EvrDatagram* _evrDatagram;   	
  };
}
#endif


