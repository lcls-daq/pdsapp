#ifndef Pds_EvrConfig_V1_hh
#define Pds_EvrConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class EvrConfig_V1 : public Serializer {
  public:
    EvrConfig_V1();
    ~EvrConfig_V1() {}
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
