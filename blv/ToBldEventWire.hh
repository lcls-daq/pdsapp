#ifndef PDS_TOBLDEVENTWIRE_HH
#define PDS_TOBLDEVENTWIRE_HH

#include <sys/uio.h>
#include "pds/utility/OutletWire.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "pds/utility/ToNetEb.hh"
#include "pds/service/GenericPoolW.hh"
#include "pds/service/Task.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/Sequence.hh"

namespace Pds {

  namespace Camera { class FrameV1; };

  class Outlet;

  class ToBldEventWire : public OutletWire,
                         public XtcIterator {
  public:
    ToBldEventWire(Outlet&        outlet,
                   int            interface, 
                   int            write_fd,
                   const BldInfo& bld,
                   unsigned       wait_us,
                   unsigned       extent);
    virtual ~ToBldEventWire();

    Transition* forward(Transition* tr);
    Occurrence* forward(Occurrence* tr);
    InDatagram* forward(InDatagram* in);
    void bind(NamedConnection, const Ins& );
    void bind(unsigned id, const Ins& node);
    void unbind(unsigned id);
    
    // Debugging
    void dump(int detail);
    void dumpHistograms(unsigned tag, const char* path);
    void resetHistograms();
	
    bool isempty() const {return true;}
  private:
    int process(Xtc* xtc);
    void _send(const Xtc* inxtc);
  private:
    virtual void _handle_config(const Xtc* ) = 0;
    virtual void _attach_config(InDatagram*) = 0;
  protected:
    const BldInfo&                    _bld;

  private:
    ToNetEb                           _postman;
    int                               _write_fd;
    GenericPoolW                      _pool;
    Sequence                          _seq;
    unsigned                          _wait_us;
    Task*                             _task;
  };
}

#endif
