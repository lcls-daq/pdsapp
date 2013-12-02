#ifndef Pds_EpixSamplerConfig_hh
#define Pds_EpixSamplerConfig_hh

#include "pdsapp/config/Serializer.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include <stdint.h>
#include <QtGui/QPushButton>
#include <QtCore/QObject>

namespace Pds_ConfigDb {

  class EpixSamplerConfig : public Serializer {
  public:
    EpixSamplerConfig();
    ~EpixSamplerConfig() {};
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  public:
    NumericInt<uint32_t>*       _reg[EpixSamplerConfigShadow::NumberOfRegisters];
  };

};

#endif
