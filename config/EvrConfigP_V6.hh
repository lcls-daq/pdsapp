#ifndef Pds_EvrConfigP_V6_hh
#define Pds_EvrConfigP_V6_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace EvrConfig_V6 {
    class EvrConfigP : public Serializer {
    public:
      EvrConfigP();
      ~EvrConfigP() {}
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
