#ifndef UsdUsbConfig_hh
#define UsdUsbConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class UsdUsbConfig : public Serializer {
  public:
    UsdUsbConfig();
    ~UsdUsbConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
