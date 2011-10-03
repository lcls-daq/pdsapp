#ifndef Pds_Gsc16aiConfig_hh
#define Pds_Gsc16aiConfig_hh

#include "pdsapp/config/Serializer.hh"

class QCheckBox;

namespace Pds_ConfigDb
{
  class Gsc16aiConfig;
}

class Pds_ConfigDb::Gsc16aiConfig
  : public Serializer
{
  public:
    Gsc16aiConfig();
    ~Gsc16aiConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;

  private:
    class Private_Data;
    Private_Data* _private_data;
};

#endif
