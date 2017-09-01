#ifndef JungfrauConfig_hh
#define JungfrauConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class JungfrauConfig : public Serializer {
  public:
    JungfrauConfig(bool expert_mode=false);
    ~JungfrauConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
