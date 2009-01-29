#ifndef Pds_FexFrameServer_hh
#define Pds_FexFrameServer_hh
//=============================================
//  class FexFrameServer
//
//  An instanciation of EbServer that provides
//  feature extracted camera frames for the event builder.
//=============================================

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "FrameServerMsg.hh"

namespace PdsLeutron {
  class Opal1000;
};

namespace Pds {

  class DmaSplice;
  class Frame;
  class CameraFexConfig;
  class TwoDMoments;

  class FexFrameServer : public EbServer, public EbCountSrv {
  public:
    FexFrameServer (const Src&,
		    PdsLeutron::Opal1000&,
		    DmaSplice&);
    ~FexFrameServer();
  public:
    void        Config(const CameraFexConfig&);
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
    int _queue_frame( const Frame&    frame,
		      FrameServerMsg* msg,
		      ZcpFragment&    zfo );
    int _queue_fex  ( const TwoDMoments&,
		      FrameServerMsg* msg,
		      ZcpFragment&    zfo );
    int _queue_fex_and_frame( const TwoDMoments&,
			      const Frame&    frame,
			      FrameServerMsg* msg,
			      ZcpFragment&    zfo );
  private:    
    PdsLeutron::Opal1000&  _camera;
    DmaSplice& _splice;
    bool       _more;
    unsigned   _offset;
    int        _fd[2];
    Xtc        _xtc;
    unsigned   _count;
    LinkedList<FrameServerMsg> _msg_queue;
    const CameraFexConfig* _config;
  };
}

#endif
