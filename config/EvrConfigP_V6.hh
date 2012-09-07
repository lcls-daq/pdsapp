#ifndef Pds_EvrConfig_V6_hh
#define Pds_EvrConfig_V6_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class EvrConfigP_V6 : public Serializer {
  public:
    EvrConfigP_V6();
    ~EvrConfigP_V6() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
