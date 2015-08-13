#ifndef Pds_MonReqServer_hh
#define Pds_MonReqServer_hh

#include "pds/utility/Appliance.hh"

namespace Pds {
  
  class MonReqServer : public Appliance {
  public:
    MonReqServer();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  };
};

#endif
