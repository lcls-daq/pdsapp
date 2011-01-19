#ifndef Pds_AcqTdcConfig_hh
#define Pds_AcqTdcConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class AcqTdcConfig : public Serializer {
  public:
    AcqTdcConfig();
    ~AcqTdcConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
