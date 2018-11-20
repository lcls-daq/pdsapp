#ifndef PdsApp_PgpDevice_hh
#define PdsApp_PgpDevice_hh

#include "pdsapp/devtest/Device.hh"

namespace Pds {
  namespace Pgp { 
    class Destination;
    namespace SrpV3 { class Protocol; }
  }
}

namespace PdsApp {
  class PgpDevice : public Device {
  public:
    PgpDevice(Pds::Pgp::SrpV3::Protocol&,
              unsigned dst,
              unsigned addr,
              unsigned chunk=256);
    ~PgpDevice();
  public:
    void      rawWrite(unsigned offset, 
                       unsigned v);
    void      rawWrite(unsigned offset, 
                       unsigned* arg1);
    unsigned  rawRead (unsigned offset);
    uint32_t* rawRead (unsigned offset, unsigned nword);
  private:
    char* _p;
    char* _data;
  };
}

#endif
