#ifndef ArchonConfig_V2_hh
#define ArchonConfig_V2_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class ArchonConfig_V2 : public Serializer {
  public:
    ArchonConfig_V2(bool expert_mode=false);
    ~ArchonConfig_V2() {}

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
