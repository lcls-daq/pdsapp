#ifndef Pds_AliasConfig_hh
#define Pds_AliasConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class AliasConfig : public Serializer {
  public:
    AliasConfig();
    ~AliasConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
