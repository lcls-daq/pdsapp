#ifndef Pds_PVControl_hh
#define Pds_PVControl_hh

#include "pds/config/ControlConfigType.hh"

#include <list>

namespace Pds {

  class ControlCA;

  class PVControl {
  public:
    PVControl ();
    virtual ~PVControl();
  public:
    void configure(const ControlConfigType&);
  private:
    std::list<ControlCA*> _channels;
  };
};

#endif
