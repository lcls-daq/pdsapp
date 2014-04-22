// $Id$

#include "pdsapp/config/RayonixConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/psddl/rayonix.ddl.h"
#include "pds/config/RayonixConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  // these must end in NULL
  static const char* readoutMode_to_name[] = { "Standard", "High gain", "Low noise", "HDR", NULL };

  static const char* res_to_name_expert[] = { "1920x1920 (binning 2x2)", 
                                              "1280x1280 (binning 3x3)", 
                                              "960x960 (binning 4x4)", 
                                              "768x768 (binning 5x5)", 
                                              "640x640 (binning 6x6)", 
                                              "480x480 (binning 8x8)", 
                                              "384x384 (binning 10x10)",
                                              NULL };
  static const char* res_to_name[] =        { "1920x1920 (binning 2x2)", 
                                              "960x960 (binning 4x4)", 
                                              NULL };

  static const char* dark_to_name_expert[] = { "Keep current background", "Update background on config", NULL };
  static const char* dark_to_name[] =        { "Keep current background", "Update background on config", NULL };

  static const char* raw_to_name_expert[] = { "Corrected", "Raw", NULL };
  static const char* raw_to_name[] =        { "Corrected", NULL };

  static const char* trigger_to_name[] = { "Frame transfer (integrate always)",
                                           "Bulb mode (integrate during pulse)", NULL };

  // ------- expert mode ---------
  class RayonixExpertConfig::Private_Data {
  public:
    enum Mode       { standardMode=0 };
    enum Resolution { res1920x1920=0, 
                      res1280x1280=1, 
                      res960x960=2,
                      res768x768=3, 
                      res640x640=4, 
                      res480x480=5, 
                      res384x384=6}; 
    enum Dark       { keepDark=0, newDark=1 };
    enum Raw        { correctedFrames=0, rawFrames=1 };
    enum Trigger    { frameTransferMode=0, bulbMode=1 };

    Private_Data() :
      _readoutMode    ("Readout mode", (Pds::Rayonix::ConfigV2::ReadoutMode)0, readoutMode_to_name),
      _resolution     ("Resolution",      res1920x1920,           res_to_name_expert),
      _binning_f      (0),    // derived from _resolution
      _binning_s      (0),    // derived from _resolution
      _testPattern    ("Test pattern",          0,            -1,32767),
      _exposure_ms    ("Exposure (ms)",         0,            0,0),   // not used
      _trigger        ("Trigger mode",          frameTransferMode, trigger_to_name),
      _rawMode        ("Frame type",            correctedFrames, raw_to_name_expert),
      _darkFlag       ("Dark frame collection", keepDark, dark_to_name_expert)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_readoutMode);
      pList.insert(&_resolution);
      pList.insert(&_testPattern);
      pList.insert(&_trigger);
      pList.insert(&_rawMode);
      pList.insert(&_darkFlag);
    }

    uint32_t binningEnum2int(uint32_t in)
    {
      switch (in) {
        case res1920x1920: return (2);
        case res1280x1280: return(3); 
        case res960x960: return (4);
        case res768x768: return(5);
        case res640x640: return (6);
        case res480x480: return(8);
        case res384x384: return(10);
        default: break;
      }
      return (2);
    }

    uint32_t binning2enum(uint32_t in)
    {
      switch (in) {
        case 2: return(res1920x1920);
        case 3: return(res1280x1280); 
        case 4: return(res960x960);
        case 5: return(res768x768);
        case 6: return(res640x640);
        case 8: return(res480x480);
        case 10: return(res384x384);
        default: break;
      }
      return (2);
    }

    int pull(void* from) {
      Pds::Rayonix::ConfigV2& tc = *reinterpret_cast<Pds::Rayonix::ConfigV2*>(from);

      _binning_f = tc.binning_f();
      _binning_s = tc.binning_s();
      _resolution.value = binning2enum(_binning_f);
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

      Pds::Rayonix::ConfigV2& tc = *new(to) Pds::Rayonix::ConfigV2(
        _binning_f,
        _binning_s,
        _testPattern.value,
        _exposure_ms.value,
        _trigger.value,
        _rawMode.value,
        _darkFlag.value,
        _readoutMode.value,
        "name:0000"
        );

      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(Pds::Rayonix::ConfigV2);
    }

  public:
    Enumerated<Pds::Rayonix::ConfigV2::ReadoutMode> _readoutMode;
    Enumerated<uint32_t> _resolution;
    uint8_t _binning_f;
    uint8_t _binning_s;
    NumericInt<int16_t> _testPattern;
    NumericInt<uint16_t> _exposure_ms;
    Enumerated<uint32_t> _trigger;
    Enumerated<uint16_t> _rawMode;
    Enumerated<uint16_t> _darkFlag;
  };

  // ------- user mode ---------
  class RayonixConfig::Private_Data {
  public:
    enum Mode       { standardMode=0 };
    enum Resolution { res1920x1920=0, res960x960=1 }; 
    enum Dark       { keepDark=0, newDark=1 };
    enum Raw        { correctedFrames=0 };
    enum Trigger    { frameTransferMode=0, bulbMode=1 };

    Private_Data() :
      _readoutMode    ("Readout mode", (Pds::Rayonix::ConfigV2::ReadoutMode)0, readoutMode_to_name),
      _resolution     ("Resolution",      res1920x1920,           res_to_name),
      _binning_f      (0),    // derived from _resolution
      _binning_s      (0),    // derived from _resolution
      _testPattern    ("Test pattern",          0,            0,1),
      _exposure_ms    ("Exposure (ms)",         0,            0,0),   // not used
      _trigger        ("Trigger mode",          frameTransferMode, trigger_to_name),
      _rawMode        ("Frame type",            correctedFrames, raw_to_name),
      _darkFlag       ("Dark frame collection", keepDark, dark_to_name)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_readoutMode);
      pList.insert(&_resolution);
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
      Pds::Rayonix::ConfigV2& tc = *reinterpret_cast<Pds::Rayonix::ConfigV2*>(from);

      _binning_f = tc.binning_f();
      _binning_s = tc.binning_s();
      _resolution.value = (_binning_f == 4) ? res960x960 : res1920x1920;
      _testPattern.value = tc.testPattern();
      _exposure_ms.value = tc.exposure();
      _trigger.value = tc.trigger();
      _rawMode.value = correctedFrames;
      _darkFlag.value = tc.darkFlag();
      _readoutMode.value = tc.readoutMode();
      return tc._sizeof();
    }

    int push(void* to) {
      // restriction: binning_f == binning_s
      _binning_f = _binning_s = binningEnum2int(_resolution.value);

      Pds::Rayonix::ConfigV2& tc = *new(to) Pds::Rayonix::ConfigV2(
        _binning_f,
        _binning_s,
        _testPattern.value,
        _exposure_ms.value,
        _trigger.value,
        _rawMode.value,
        _darkFlag.value,
        _readoutMode.value,
        "name:0000"
        );

      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(Pds::Rayonix::ConfigV2);
    }

  public:
    Enumerated<Pds::Rayonix::ConfigV2::ReadoutMode> _readoutMode;
    Enumerated<uint32_t> _resolution;
    uint8_t _binning_f;
    uint8_t _binning_s;
    NumericInt<int16_t> _testPattern;
    NumericInt<uint16_t> _exposure_ms;
    Enumerated<uint32_t> _trigger;
    Enumerated<uint16_t> _rawMode;
    Enumerated<uint16_t> _darkFlag;
  };
};

using namespace Pds_ConfigDb;

// ---------- expert mode ------------
RayonixExpertConfig::RayonixExpertConfig() :
  Serializer("Rayonix_Expert_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  RayonixExpertConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  RayonixExpertConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  RayonixExpertConfig::dataSize() const {
  return _private_data->dataSize();
}

// ---------- user mode ------------
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

