#ifndef Pds_EvrConfig_V4_hh
#define Pds_EvrConfig_V4_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class EvrPulseTable_V4;

  class EvrConfig_V4 : public Serializer {
  public:
    EvrConfig_V4();
    ~EvrConfig_V4() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    EvrPulseTable_V4* _table;
  };

};

#endif
