#ifndef Pds_FrameServer_hh
#define Pds_FrameServer_hh
//=============================================
//  class FrameServer
//
//  An instanciation of EbServer that provides
//  camera frames for the event builder.
//=============================================

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/xtc/Xtc.hh"

namespace Pds {

  class Frame;
  class Opal1000;
  
  class FrameServer : public EbServer, public EbCountSrv {
  public:
    FrameServer (const Src&,
		 Opal1000&);
    ~FrameServer();
  public:
    void        post();
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const Xtc&  xtc   () const;
    bool        more  () const;
    unsigned    length() const;
    unsigned    offset() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend        (int flag = 0);
    int      fetch       (char* payload, int flags);
    int      fetch       (ZcpFragment& , int flags);
  public:
    unsigned count() const;
  private:
    Opal1000& _camera;
    bool     _more;
    unsigned _offset;
    unsigned _next;
    unsigned _length;
    int      _fd[2];
    Xtc      _xtc;
    unsigned _count;
  };
}

#endif
