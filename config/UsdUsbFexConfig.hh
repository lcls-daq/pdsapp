#ifndef UsdUsbFexConfig_hh
#define UsdUsbFexConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class UsdUsbFexConfig : public Serializer {
  public:
    UsdUsbFexConfig();
    ~UsdUsbFexConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
