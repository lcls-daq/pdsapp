// $Id$

#include "pdsapp/config/RayonixConfig_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/psddl/rayonix.ddl.h"

#include <new>

namespace Pds_ConfigDb {
   // these must end in NULL
   static const char* full_readoutMode_to_name[] = { "Standard", "High Gain", "Low Noise", "HDR", NULL };
   static const char* full_res_to_name[] = { "3840x3840 (binning 1x1)",
                                             "1920x1920 (binning 2x2)", 
                                             "1280x1280 (binning 3x3)", 
                                             "960x960 (binning 4x4)", 
                                             "768x768 (binning 5x5)", 
                                             "640x640 (binning 6x6)", 
                                             "480x480 (binning 8x8)", 
                                             "384x384 (binning 10x10)",
                                             NULL };


  // these must end in NULL
  static const char* readoutMode_to_name[] = { "Standard", "High Gain", "Low Noise", "EDR", NULL };
  static const char* res_to_name[] = { "1920x1920 (binning 2x2)", "960x960 (binning 4x4)", NULL };

  // ------- expert mode ---------
  class RayonixExpertConfig_V1::Private_Data {
  public:
    enum Resolution { res3840x3840=0, 
                      res1920x1920=1, 
                      res1280x1280=2, 
                      res960x960=3,
                      res768x768=4, 
                      res640x640=5, 
                      res480x480=6, 
                      res384x384=7}; 


    Private_Data() :
      _readoutMode    ("Readout mode", Pds::Rayonix::ConfigV1::Standard, full_readoutMode_to_name),
      _resolution     ("Resolution",      res1920x1920,           full_res_to_name),
      _binning_f      (0),    // derived from _resolution
      _binning_s      (0),    // derived from _resolution
      _exposure_ms    ("Exposure (ms)",         0,            0,10000),
      _trigger        ("Trigger (hex)",         0,            0,0xffffffff, Hex),
      _rawMode        ("Raw mode (0 or 1)",     0,            0,1),
      _darkFlag       ("Dark flag (0 or 1)",    0,            0,1)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_readoutMode);
      pList.insert(&_resolution);
      pList.insert(&_exposure_ms);
      pList.insert(&_trigger);
      pList.insert(&_rawMode);
      pList.insert(&_darkFlag);
    }

    uint32_t binningEnum2int(uint32_t in)
    {
      switch (in) {
        case res3840x3840: return(1);
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
        case 1: return(res3840x3840);
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
      Pds::Rayonix::ConfigV1& tc = *reinterpret_cast<Pds::Rayonix::ConfigV1*>(from);

      _binning_f = tc.binning_f();
      _binning_s = tc.binning_s();
      _resolution.value = binning2enum(_binning_f);
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

      Pds::Rayonix::ConfigV1& tc = *new(to) Pds::Rayonix::ConfigV1(
        _binning_f,
        _binning_s,
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
      return sizeof(Pds::Rayonix::ConfigV1);
    }

  public:
    Enumerated<Pds::Rayonix::ConfigV1::ReadoutMode> _readoutMode;
    Enumerated<uint32_t> _resolution;
    uint8_t _binning_f;
    uint8_t _binning_s;
    NumericInt<uint16_t> _exposure_ms;
    NumericInt<uint32_t> _trigger;
    NumericInt<uint16_t> _rawMode;
    NumericInt<uint16_t> _darkFlag;
  };


  // ----------- user mode --------
  class RayonixConfig_V1::Private_Data {
  public:
    enum Resolution { res1920x1920=0, res960x960=1 }; 

    Private_Data() :
      _readoutMode    ("Readout mode", Pds::Rayonix::ConfigV1::Standard, readoutMode_to_name),
      _resolution     ("Resolution",      res1920x1920,           res_to_name),
      _binning_f      (0),    // derived from _resolution
      _binning_s      (0),    // derived from _resolution
      _exposure_ms    ("Exposure (ms)",         0,            0,10000),
      _trigger        ("Trigger (hex)",         0,            0,0xffffffff, Hex),
      _rawMode        ("Raw mode (0 or 1)",     0,            0,1),
      _darkFlag       ("Dark flag (0 or 1)",    0,            0,1)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_readoutMode);
      pList.insert(&_resolution);
      pList.insert(&_exposure_ms);
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
      Pds::Rayonix::ConfigV1& tc = *reinterpret_cast<Pds::Rayonix::ConfigV1*>(from);

      _binning_f = tc.binning_f();
      _binning_s = tc.binning_s();
      _resolution.value = (_binning_f == 4) ? res960x960 : res1920x1920;
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

      Pds::Rayonix::ConfigV1& tc = *new(to) Pds::Rayonix::ConfigV1(
        _binning_f,
        _binning_s,
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
      return sizeof(Pds::Rayonix::ConfigV1);
    }

  public:
    Enumerated<Pds::Rayonix::ConfigV1::ReadoutMode> _readoutMode;
    Enumerated<uint32_t> _resolution;
    uint8_t _binning_f;
    uint8_t _binning_s;
    NumericInt<uint16_t> _exposure_ms;
    NumericInt<uint32_t> _trigger;
    NumericInt<uint16_t> _rawMode;
    NumericInt<uint16_t> _darkFlag;
  };
};

using namespace Pds_ConfigDb;

// ------------ expert mode ------------
RayonixExpertConfig_V1::RayonixExpertConfig_V1() :
  Serializer("Rayonix_Expert_Config_V1"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  RayonixExpertConfig_V1::readParameters (void* from) {
  return _private_data->pull(from);
}

int  RayonixExpertConfig_V1::writeParameters(void* to) {
  return _private_data->push(to);
}

int  RayonixExpertConfig_V1::dataSize() const {
  return _private_data->dataSize();
}

// ------------ user mode ------------

RayonixConfig_V1::RayonixConfig_V1() :
  Serializer("Rayonix_Config_V1"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  RayonixConfig_V1::readParameters (void* from) {
  return _private_data->pull(from);
}

int  RayonixConfig_V1::writeParameters(void* to) {
  return _private_data->push(to);
}

int  RayonixConfig_V1::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

