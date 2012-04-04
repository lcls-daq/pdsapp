#ifndef Pds_OceanOpticsConfig_hh
#define Pds_OceanOpticsConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class OceanOpticsConfig : public Serializer {
  public:
    OceanOpticsConfig();
    ~OceanOpticsConfig() {}
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
