#ifndef Pds_OrcaConfig_hh
#define Pds_OrcaConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class OrcaConfig : public Serializer {
  public:
    OrcaConfig();
    ~OrcaConfig() {}
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
