#ifndef JungfrauConfig_V1_hh
#define JungfrauConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class JungfrauConfig_V1 : public Serializer {
  public:
    JungfrauConfig_V1();
    ~JungfrauConfig_V1() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
  };
};

#endif
