#ifndef JungfrauConfig_V3_hh
#define JungfrauConfig_V3_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class JungfrauConfig_V3 : public Serializer {
  public:
    JungfrauConfig_V3(bool expert_mode=false);
    ~JungfrauConfig_V3() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
