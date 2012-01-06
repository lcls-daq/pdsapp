#include "pdsapp/config/PhasicsConfig.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/PhasicsConfigType.hh"

#include <new>

namespace Pds_ConfigDb {


  PhasicsConfig::PhasicsConfig() :
                Serializer("Phasics_Config") {
    for (uint32_t i=0; i<PhasicsConfigType::NumberOfRegisters; i++) {
      _reg[i] = new NumericInt<uint32_t>(
          PhasicsConfigType::name((PhasicsConfigType::Registers) i),
          PhasicsConfigType::defaultValue((PhasicsConfigType::Registers) i),
          PhasicsConfigType::rangeLow((PhasicsConfigType::Registers) i),
          PhasicsConfigType::rangeHigh((PhasicsConfigType::Registers) i),
          Decimal
      );
      pList.insert(_reg[i]);
    }
    name("PHASICS Configuration");
  }

  int PhasicsConfig::readParameters(void* from) { // pull "from xtc"
    PhasicsConfigType& phasicsConf = *new(from) PhasicsConfigType;
    for (uint32_t i=0; i<PhasicsConfigType::NumberOfRegisters; i++) {
      _reg[i]->value = phasicsConf.get((PhasicsConfigType::Registers) i);
    }
    return sizeof(PhasicsConfigType);
  }

  int PhasicsConfig::writeParameters(void* to) {
    PhasicsConfigType& phasicsConf = *new(to) PhasicsConfigType();
    for (uint32_t i=0; i<PhasicsConfigType::NumberOfRegisters; i++) {
      phasicsConf.set((PhasicsConfigType::Registers) i, _reg[i]->value);
    }
    return sizeof(PhasicsConfigType);
  }

  int PhasicsConfig::dataSize() const { return sizeof(PhasicsConfigType); }

};

#include "Parameters.icc"
