#ifndef Pds_FliConfig_hh
#define Pds_FliConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class FliConfig : public Serializer {
  public:
    FliConfig();
    ~FliConfig() {}
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
