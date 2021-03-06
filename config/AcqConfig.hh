#ifndef Pds_AcqConfig_hh
#define Pds_AcqConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class AcqConfig : public Serializer {
  public:
    AcqConfig();
    ~AcqConfig() {}
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
