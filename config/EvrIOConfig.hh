#ifndef Pds_EvrIOConfig_hh
#define Pds_EvrIOConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class EvrIOConfig : public Serializer {
  public:
    EvrIOConfig();
    ~EvrIOConfig() {}
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
