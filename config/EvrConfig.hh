#ifndef Pds_EvrConfig_hh
#define Pds_EvrConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class EvrConfig : public Serializer {
  public:
    EvrConfig();
    ~EvrConfig() {}
  public:
    bool readParameters (void* from);
    int  writeParameters(void* to);
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
