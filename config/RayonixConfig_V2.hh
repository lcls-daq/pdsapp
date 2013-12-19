#ifndef Pds_RayonixConfig_V2_hh
#define Pds_RayonixConfig_V2_hh

#include "pdsapp/config/Serializer.hh"

class QCheckBox;

namespace Pds_ConfigDb
{
  class RayonixExpertConfig_V2;
  class RayonixConfig_V2;
}

class Pds_ConfigDb::RayonixExpertConfig_V2
  : public Serializer
{
  public:
    RayonixExpertConfig_V2();
    ~RayonixExpertConfig_V2() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

class Pds_ConfigDb::RayonixConfig_V2
  : public Serializer
{
  public:
    RayonixConfig_V2();
    ~RayonixConfig_V2() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

#endif
