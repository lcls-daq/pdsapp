#ifndef Pds_EvrConfig_V5_hh
#define Pds_EvrConfig_V5_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace EvrConfig_V5 {
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
      class Private_Data;
      Private_Data* _private_data;
    };
  };
};

#endif
