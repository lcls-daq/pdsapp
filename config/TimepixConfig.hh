#ifndef Pds_TimepixConfig_hh
#define Pds_TimepixConfig_hh

#include "pdsapp/config/Serializer.hh"

class QCheckBox;

namespace Pds_ConfigDb
{
  class TimepixConfig;
}

class Pds_ConfigDb::TimepixConfig
  : public Serializer
{
  public:
    TimepixConfig();
    ~TimepixConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

#endif
