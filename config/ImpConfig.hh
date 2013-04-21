#ifndef Pds_ImpConfig_hh
#define Pds_ImpConfig_hh

#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/ImpConfigType.hh"

namespace Pds_ConfigDb {

  class ImpConfig : public Serializer {
  public:
    ImpConfig();
    ~ImpConfig() {};
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  public:
    NumericInt<uint32_t>*       _reg[ImpConfigType::NumberOfRegisters];
  };

};

#endif
