#ifndef Pds_pnCCDConfig_hh
#define Pds_pnCCDConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class pnCCDConfig : public Serializer {
  public:
    pnCCDConfig();
    ~pnCCDConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
