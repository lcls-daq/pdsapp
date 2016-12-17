#ifndef ArchonConfig_hh
#define ArchonConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class ArchonConfig : public Serializer {
  public:
    ArchonConfig();
    ~ArchonConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
