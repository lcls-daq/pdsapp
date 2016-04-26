#ifndef Pds_TestApp_hh
#define Pds_TestApp_hh

#include "pds/utility/Appliance.hh"

namespace Pds {
  class TestApp : public Appliance {
  public:
    TestApp();
    ~TestApp();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  };
};

#endif
