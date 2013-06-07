#ifndef Pds_ToOpalBldEventWire_hh
#define Pds_ToOpalBldEventWire_hh

#include "pdsapp/blv/ToBldEventWire.hh"

#include "pds/config/Opal1kConfigType.hh"

namespace Pds {
  class ToOpalBldEventWire : public ToBldEventWire {
  public:
    ToOpalBldEventWire(Outlet&        outlet,
                      int            interface, 
                      int            write_fd,
                      const BldInfo& bld,
                      unsigned       wait_us);
    ~ToOpalBldEventWire();
  private:
    void _handle_config(const Xtc* );
    void _attach_config(InDatagram*);
  private:
    Opal1kConfigType       _camConfig;
  };
};

#endif
