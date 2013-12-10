// $Id$

#include "pdsapp/config/RayonixConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/psddl/rayonix.ddl.h"
#include "pds/config/RayonixConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  // these must end in NULL
  static const char* readoutMode_to_name[] = { "Standard", "High Gain", "Low Noise", "EDR", NULL };
  static const char* res_to_name[] = { "1920x1920 (binning 2x2)", "960x960 (binning 4x4)", NULL };

  class RayonixConfig::Private_Data {
  public:
    enum Resolution { res1920x1920=0, res960x960=1 }; 

    Private_Data() :
      _readoutMode    ("Readout mode", RayonixConfigType::Standard, readoutMode_to_name),
      _resolution     ("Resolution",      res1920x1920,           res_to_name),
      _binning_f      (0),    // derived from _resolution
      _binning_s      (0),    // derived from _resolution
      _testPattern    ("Test pattern",          0,            0,1),
      _exposure_ms    ("Exposure (ms)",         0,            0,10000),
      _trigger        ("Trigger (hex)",         0,            0,0xffffffff, Hex),
      _rawMode        ("Raw mode (0 or 1)",     0,            0,1),
      _darkFlag       ("Dark flag (0 or 1)",    0,            0,1)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_readoutMode);
      pList.insert(&_resolution);
      pList.insert(&_exposure_ms);
      pList.insert(&_testPattern);
      pList.insert(&_trigger);
      pList.insert(&_rawMode);
      pList.insert(&_darkFlag);
    }

    uint32_t binningEnum2int(uint32_t in)
    {
      switch (in) {
        case res1920x1920: return (2);
        case res960x960: return (4);
        default: break;
      }
      return (2);
    }

    int pull(void* from) {
      RayonixConfigType& tc = *reinterpret_cast<RayonixConfigType*>(from);

      _binning_f = tc.binning_f();
      _binning_s = tc.binning_s();
      _resolution.value = (_binning_f == 4) ? res960x960 : res1920x1920;
      _testPattern.value = tc.testPattern();
      _exposure_ms.value = tc.exposure();
      _trigger.value = tc.trigger();
      _rawMode.value = tc.rawMode();
      _darkFlag.value = tc.darkFlag();
      _readoutMode.value = tc.readoutMode();
      return tc._sizeof();
    }

    int push(void* to) {
      // restriction: binning_f == binning_s
      _binning_f = _binning_s = binningEnum2int(_resolution.value);

      RayonixConfigType& tc = *new(to) RayonixConfigType(
        _binning_f,
        _binning_s,
        _exposure_ms.value,
        _testPattern.value,
        _trigger.value,
        _rawMode.value,
        _darkFlag.value,
        _readoutMode.value,
        "name:0000"
        );

      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(RayonixConfigType);
    }

  public:
    Enumerated<RayonixConfigType::ReadoutMode> _readoutMode;
    Enumerated<uint32_t> _resolution;
    uint8_t _binning_f;
    uint8_t _binning_s;
    NumericInt<int16_t> _testPattern;
    NumericInt<uint16_t> _exposure_ms;
    NumericInt<uint32_t> _trigger;
    NumericInt<uint16_t> _rawMode;
    NumericInt<uint16_t> _darkFlag;
  };
};

using namespace Pds_ConfigDb;

RayonixConfig::RayonixConfig() :
  Serializer("Rayonix_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  RayonixConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  RayonixConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  RayonixConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

