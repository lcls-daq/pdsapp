#include "ImpConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/ImpConfigType.hh"

#include <new>

namespace Pds_ConfigDb {


  ImpConfig::ImpConfig() : Serializer("Imp_Config")
                 {
    for (uint32_t i=0; i<ImpConfigType::NumberOfRegisters; i++) {
      _reg[i] = new NumericInt<uint32_t>(
          ImpConfigType::name((ImpConfigType::Registers) i),
          ImpConfigType::defaultValue((ImpConfigType::Registers) i),
          ImpConfigType::rangeLow((ImpConfigType::Registers) i),
          ImpConfigType::rangeHigh((ImpConfigType::Registers) i),
          Hex
      );
      pList.insert(_reg[i]);
    }
    name("IMP Configuration");
  }

  int ImpConfig::readParameters(void* from) { // pull "from xtc"
    ImpConfigType& impConf = *new(from) ImpConfigType;
    for (uint32_t i=0; i<ImpConfigType::NumberOfRegisters; i++) {
      _reg[i]->value = impConf.get((ImpConfigType::Registers) i);
    }
    return sizeof(ImpConfigType);
  }

  int ImpConfig::writeParameters(void* to) {
    ImpConfigType& impConf = *new(to) ImpConfigType();
    for (uint32_t i=0; i<ImpConfigType::NumberOfRegisters; i++) {
      impConf.set((ImpConfigType::Registers) i, _reg[i]->value);
    }
    return sizeof(ImpConfigType);
  }

  int ImpConfig::dataSize() const { return sizeof(ImpConfigType); }

};

#include "Parameters.icc"
