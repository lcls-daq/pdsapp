#ifndef Pds_EvrConfig_V4_hh
#define Pds_EvrConfig_V4_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace EvrConfig_V4 {
    class EvrPulseTable_V4;

    class EvrConfig : public Serializer {
    public:
      EvrConfig();
      ~EvrConfig() {}
    public:
      int  readParameters (void* from);
      int  writeParameters(void* to);
      int  dataSize() const;
      bool validate();
    private:
      EvrPulseTable_V4* _table;
    };
  };
};

#endif
