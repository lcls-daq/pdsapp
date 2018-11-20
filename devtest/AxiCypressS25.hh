#ifndef PdsApp_AxiCypressS25_hh
#define PdsApp_AxiCypressS25_hh

#include "pdsapp/devtest/AxiMicronN25Q.hh"

namespace PdsApp {
  class AxiCypressS25 : public AxiMicronN25Q {
  public:
    AxiCypressS25(const char* mcsfile,
                  Device&     dev);
  public:
    void     resetFlash();
    void     waitForFlashReady();
    unsigned getFlagStatusReg();
  };
}

#endif
