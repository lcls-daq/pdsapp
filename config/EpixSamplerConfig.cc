#include "pdsapp/config/EpixSamplerConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/EpixSamplerConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  EpixSamplerConfig::EpixSamplerConfig() : Serializer("EpixSampler_Config") {
    for (uint32_t i=0; i<EpixSamplerConfigShadow::NumberOfRegisters; i++) {
      _reg[i] = new NumericInt<uint32_t>(
          EpixSamplerConfigShadow::name((EpixSamplerConfigShadow::Registers) i),
          EpixSamplerConfigShadow::defaultValue((EpixSamplerConfigShadow::Registers) i),
          EpixSamplerConfigShadow::rangeLow((EpixSamplerConfigShadow::Registers) i),
          EpixSamplerConfigShadow::rangeHigh((EpixSamplerConfigShadow::Registers) i),
          Hex
      );
      pList.insert(_reg[i]);
    }
    name("EPIXSAMPLER Configuration");
  }

  int EpixSamplerConfig::readParameters(void* from) { // pull "from xtc"
    EpixSamplerConfigShadow& epixConf = *new(from) EpixSamplerConfigShadow();
    for (uint32_t i=0; i<EpixSamplerConfigShadow::NumberOfRegisters; i++) {
      _reg[i]->value = epixConf.get((EpixSamplerConfigShadow::Registers) i);
    }
    return sizeof(EpixSamplerConfigShadow);
  }

  int EpixSamplerConfig::writeParameters(void* to) {
    EpixSamplerConfigShadow& epixConf = *new(to) EpixSamplerConfigShadow(true);
    for (uint32_t i=0; i<EpixSamplerConfigShadow::NumberOfRegisters; i++) {
      epixConf.set((EpixSamplerConfigShadow::Registers) i, _reg[i]->value);
    }
    return sizeof(EpixSamplerConfigShadow);
  }

  int EpixSamplerConfig::dataSize() const { return sizeof(EpixSamplerConfigType); }

};

#include "Parameters.icc"
