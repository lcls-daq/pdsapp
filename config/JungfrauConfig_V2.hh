#ifndef JungfrauConfig_V2_hh
#define JungfrauConfig_V2_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class JungfrauConfig_V2 : public Serializer {
  public:
    JungfrauConfig_V2(bool expert_mode=false);
    ~JungfrauConfig_V2() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
