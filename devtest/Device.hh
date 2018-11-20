#ifndef PdsApp_Device_hh
#define PdsApp_Device_hh

#include <stdint.h>

namespace PdsApp {
  class Device {
  public:
    virtual ~Device() {}
  public:
    virtual void      rawWrite(unsigned offset, 
                               unsigned v) = 0;
    virtual unsigned  rawRead (unsigned offset) = 0;
  public:
    virtual void      rawWrite(unsigned offset, 
                               unsigned* arg1) = 0;
    virtual uint32_t* rawRead (unsigned offset,
                               unsigned nword) = 0;
  };
}

#endif
