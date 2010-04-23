#ifndef Pds_FccdConfig_hh
#define Pds_FccdConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class FccdConfig : public Serializer {
  public:
    FccdConfig();
    ~FccdConfig() {}
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
