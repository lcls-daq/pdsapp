#include "pdsapp/config/FliConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/FliConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  class FliConfig::Private_Data {
  public:
    Private_Data() :
      _uWidth               ("Width",               16, 1,      4152),
      _uHeight              ("Height",              16, 1,      4128),
      _uOrgX                ("Orgin X",             0,    0,    4151),
      _uOrgY                ("Orgin Y",             0,    0,    4127),
      _uBinX                ("Binning X",           1,    1,    16),
      _uBinY                ("Binning Y",           1,    1,    16),
      // Note: Here the min exposure time need to set 9.99e-4 to allow user to input 1e-3, due to floating points imprecision
      _f32ExposureTime      ("Exposure time (sec)", 1e-3, 9.99e-4, 3600),
      _f32CoolingTemp       ("Cooling Temp (C)",    25,   -300,  25),      
      _u8GainIndex          ("Gain Index",          1,    0,    5),
      _u8ReadoutSpeedIndex  ("Readout Speed",       1,    0,    5),
      _u16ExposureEventCode ("Exposure Event Code", 1,    1,    255),
      _u32NumDelayShots     ("Number Delay Shots",  1,    0,    0x7FFFFFFF)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_uWidth);
      pList.insert(&_uHeight);
      pList.insert(&_uOrgX);
      pList.insert(&_uOrgY);
      pList.insert(&_uBinX);
      pList.insert(&_uBinY);
      pList.insert(&_f32ExposureTime);
      pList.insert(&_f32CoolingTemp);
      pList.insert(&_u8GainIndex);
      pList.insert(&_u8ReadoutSpeedIndex);
      pList.insert(&_u16ExposureEventCode);
      pList.insert(&_u32NumDelayShots);
    }

    int pull(void* from) {
      FliConfigType& tc = *new(from) FliConfigType;
      _uWidth               .value = tc.width   ();
      _uHeight              .value = tc.height  ();
      _uOrgX                .value = tc.orgX    ();
      _uOrgY                .value = tc.orgY    ();
      _uBinX                .value = tc.binX    ();
      _uBinY                .value = tc.binY    ();
      _f32ExposureTime      .value = tc.exposureTime();
      _f32CoolingTemp       .value = tc.coolingTemp ();
      _u8GainIndex          .value = tc.gainIndex();
      _u8ReadoutSpeedIndex  .value = tc.readoutSpeedIndex();
      _u16ExposureEventCode .value = tc.exposureEventCode();
      _u32NumDelayShots     .value = tc.numDelayShots();
      return tc._sizeof();
    }

    int push(void* to) {
      FliConfigType& tc = *new(to) FliConfigType(
        _uWidth               .value,
        _uHeight              .value,
        _uOrgX                .value,
        _uOrgY                .value,
        _uBinX                .value,
        _uBinY                .value,
        _f32ExposureTime      .value,
        _f32CoolingTemp       .value,
        _u8GainIndex          .value,
        _u8ReadoutSpeedIndex  .value,
        _u16ExposureEventCode .value,
        _u32NumDelayShots     .value
      );
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(FliConfigType);
    }

  public:
    NumericInt<uint32_t>    _uWidth;
    NumericInt<uint32_t>    _uHeight;    
    NumericInt<uint32_t>    _uOrgX;
    NumericInt<uint32_t>    _uOrgY;    
    NumericInt<uint32_t>    _uBinX;
    NumericInt<uint32_t>    _uBinY;    
    NumericFloat<float>     _f32ExposureTime;    
    NumericFloat<float>     _f32CoolingTemp;        
    NumericInt<uint8_t>     _u8GainIndex;        
    NumericInt<uint8_t>     _u8ReadoutSpeedIndex;        
    NumericInt<uint16_t>    _u16ExposureEventCode;
    NumericInt<uint32_t>    _u32NumDelayShots;
  };
};

  // !! for reference: from pdsdata/psddl/fli.ddl.h
  //uint32_t          _uWidth, _uHeight;
  //uint32_t          _uOrgX,  _uOrgY;
  //uint32_t          _uBinX,  _uBinY;
  //float             _f32ExposureTime;
  //float             _f32CoolingTemp;
  //uint32_t          _u32ReadoutSpeedIndex;
  //uint16_t          _u16ExposureEventCode;
  //uint16_t          _u16DelayMode;

using namespace Pds_ConfigDb;

FliConfig::FliConfig() : 
  Serializer("fli_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  FliConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  FliConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  FliConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

