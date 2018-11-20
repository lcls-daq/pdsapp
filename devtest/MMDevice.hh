#ifndef PdsApp_MMDevice_hh
#define PdsApp_MMDevice_hh

#include "pdsapp/devtest/Device.hh"

namespace PdsApp {
  class MMDevice : public Device {
  public:
    MMDevice(char*    base,
             unsigned chunk=256);
    ~MMDevice();
  public:
    void      rawWrite(unsigned offset, 
                       unsigned v);
    void      rawWrite(unsigned offset, 
                       unsigned* arg1);
    unsigned  rawRead (unsigned offset);
    uint32_t* rawRead (unsigned offset, unsigned nword);
  private:
    volatile char* _p;
    char* _data;
  };
}

#endif
