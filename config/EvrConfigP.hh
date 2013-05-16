#ifndef Pds_EvrConfigP_hh
#define Pds_EvrConfigP_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class EvrConfigP : public Serializer {
  public:
    EvrConfigP();
    ~EvrConfigP();
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
