#ifndef Pds_PimImageConfig_hh
#define Pds_PimImageConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class PimImageConfig : public Serializer {
  public:
    PimImageConfig();
    ~PimImageConfig() {}
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
