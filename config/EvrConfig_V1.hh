#ifndef Pds_EvrConfig_V1_hh
#define Pds_EvrConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace EvrConfig_V1 {

    class EvrConfig : public Serializer {
    public:
      EvrConfig();
      ~EvrConfig() {}
    public:
      int  readParameters (void* from);
      int  writeParameters(void* to);
      int  dataSize() const;
    private:
      class Private_Data;
      Private_Data* _private_data;
    };
  };
};

#endif
