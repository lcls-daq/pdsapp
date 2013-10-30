#ifndef Pds_RayonixConfig_hh
#define Pds_RayonixConfig_hh

#include "pdsapp/config/Serializer.hh"

class QCheckBox;

namespace Pds_ConfigDb
{
  class RayonixConfig;
}

class Pds_ConfigDb::RayonixConfig
  : public Serializer
{
  public:
    RayonixConfig();
    ~RayonixConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

#endif
