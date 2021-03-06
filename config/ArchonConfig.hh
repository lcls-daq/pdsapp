#ifndef ArchonConfig_hh
#define ArchonConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class ArchonConfig : public Serializer {
  public:
    ArchonConfig(bool expert_mode=false);
    ~ArchonConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
    bool validate();
  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
