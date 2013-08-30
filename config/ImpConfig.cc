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
      _reg[i] = new NumericInt<uint32_t>(Pds::ImpConfig::name        ((ImpConfigType::Registers) i),
                                         Pds::ImpConfig::defaultValue((ImpConfigType::Registers) i),
                                         Pds::ImpConfig::rangeLow    ((ImpConfigType::Registers) i),
                                         Pds::ImpConfig::rangeHigh   ((ImpConfigType::Registers) i),
                                         Hex);
      pList.insert(_reg[i]);
    }
    name("IMP Configuration");
  }

  int ImpConfig::readParameters(void* from) { // pull "from xtc"
    ImpConfigType& impConf = *reinterpret_cast<ImpConfigType*>(from);
    for (uint32_t i=0; i<ImpConfigType::NumberOfRegisters; i++) {
      _reg[i]->value = Pds::ImpConfig::get(impConf,(ImpConfigType::Registers) i);
    }
    return sizeof(ImpConfigType);
  }

  int ImpConfig::writeParameters(void* to) {
    ImpConfigType& impConf = *reinterpret_cast<ImpConfigType*>(to);
    for (uint32_t i=0; i<ImpConfigType::NumberOfRegisters; i++) {
      Pds::ImpConfig::set(impConf,(ImpConfigType::Registers) i, _reg[i]->value);
    }
    return sizeof(ImpConfigType);
  }

  int ImpConfig::dataSize() const { return sizeof(ImpConfigType); }

};

#include "Parameters.icc"
