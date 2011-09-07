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

#include "pdsdata/bld/bldData.hh"

namespace Pds {

  class Outlet;

  class ToBldEventWire : public OutletWire,
                         public XtcIterator {
  public:
    ToBldEventWire(Outlet&        outlet,
                   int            interface, 
                   int            write_fd,
                   const BldInfo& bld,
                   unsigned       wait_us);
    ~ToBldEventWire();

    virtual Transition* forward(Transition* tr);
    virtual Occurrence* forward(Occurrence* tr);
    virtual InDatagram* forward(InDatagram* in);
    virtual void bind(NamedConnection, const Ins& );
    virtual void bind(unsigned id, const Ins& node);
    virtual void unbind(unsigned id);
    
    // Debugging
    virtual void dump(int detail);
    virtual void dumpHistograms(unsigned tag, const char* path);
    virtual void resetHistograms();
	
    bool isempty() const {return true;}
  private:
    int process(Xtc* xtc);
    void _send(const Xtc* xtc,
                const Camera::FrameV1&);
  private:
    ToNetEb                           _postman;
    const BldInfo&                    _bld;
    int                               _write_fd;
    GenericPoolW                      _pool;
    Sequence                          _seq;
    unsigned                          _wait_us;
    Task*                             _task;
    Pds::Pulnix::TM6740ConfigV2       _camConfig;
    Pds::Lusi::PimImageConfigV1       _pimConfig;
  };
}

#endif
