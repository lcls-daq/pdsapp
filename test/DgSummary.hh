#ifndef Pds_DgSummary_hh
#define Pds_DgSummary_hh

#include "pds/utility/Appliance.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {
  class DgSummary : public Appliance {
  public:
    DgSummary();
    ~DgSummary();
  public:
    Transition* transitions(Transition* tr);
    InDatagram* events     (InDatagram* dg);
  private:
    GenericPool _pool;
  };
};

#endif
