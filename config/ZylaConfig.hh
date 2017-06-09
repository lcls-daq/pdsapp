#ifndef ZylaConfig_hh
#define ZylaConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class ZylaConfig : public Serializer {
  public:
    ZylaConfig(bool expert_mode=false);
    ~ZylaConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
