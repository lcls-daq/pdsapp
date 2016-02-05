#ifndef Pds_Andor3dConfig_hh
#define Pds_Andor3dConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class Andor3dConfig : public Serializer {
  public:
    Andor3dConfig();
    ~Andor3dConfig() {}
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
