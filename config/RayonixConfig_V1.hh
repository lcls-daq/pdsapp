#ifndef Pds_RayonixConfig_V1_hh
#define Pds_RayonixConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

class QCheckBox;

namespace Pds_ConfigDb
{
  class RayonixExpertConfig_V1;
  class RayonixConfig_V1;
}

class Pds_ConfigDb::RayonixExpertConfig_V1
  : public Serializer
{
  public:
    RayonixExpertConfig_V1();
    ~RayonixExpertConfig_V1() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

class Pds_ConfigDb::RayonixConfig_V1
  : public Serializer
{
  public:
    RayonixConfig_V1();
    ~RayonixConfig_V1() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

#endif
