#ifndef Pds_CamDisplay_hh
#define Pds_CamDisplay_hh

#include "pds/client/Fsm.hh"

namespace Pds {

  class MonServerManager;

  class CamDisplay : public Fsm {
  public:
    CamDisplay(MonServerManager& monsrv);
    ~CamDisplay();
  };
}

#endif
