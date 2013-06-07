#ifndef Pds_ToPimBldEventWire_hh
#define Pds_ToPimBldEventWire_hh

#include "pdsapp/blv/ToBldEventWire.hh"

#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/PimImageConfigType.hh"

namespace Pds {
  class ToPimBldEventWire : public ToBldEventWire {
  public:
    ToPimBldEventWire(Outlet&        outlet,
                      int            interface, 
                      int            write_fd,
                      const BldInfo& bld,
                      unsigned       wait_us);
    ~ToPimBldEventWire();
  private:
    void _handle_config(const Xtc* );
    void _attach_config(InDatagram*);
  private:
    TM6740ConfigType       _camConfig;
    PimImageConfigType     _pimConfig;
  };
};

#endif
