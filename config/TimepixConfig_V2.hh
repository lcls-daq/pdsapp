#ifndef Pds_TimepixConfig_V2_hh
#define Pds_TimepixConfig_V2_hh

#include "pdsapp/config/Serializer.hh"

class QCheckBox;

namespace Pds_ConfigDb
{
  class TimepixExpertConfig_V2;
  class TimepixConfig_V2;
}

class Pds_ConfigDb::TimepixExpertConfig_V2
  : public Serializer
{
  public:
    TimepixExpertConfig_V2();
    ~TimepixExpertConfig_V2() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

class Pds_ConfigDb::TimepixConfig_V2
  : public Serializer
{
  public:
    TimepixConfig_V2();
    ~TimepixConfig_V2() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

#endif
