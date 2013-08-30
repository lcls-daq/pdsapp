#include "pdsapp/config/AndorConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/AndorConfigType.hh"

#include <new>

namespace Pds_ConfigDb {  
  class AndorConfig::Private_Data {
  static const char*  lsEnumFanMode[];
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
      _f32CoolingTemp       ("Cooling Temp (C)",    25,   -100,  25),      
      _enumFanMode          ("Fan Mode",            AndorConfigType::ENUM_FAN_FULL, lsEnumFanMode),
      _enumBaselineClamp    ("Baseline Clamp",      Enums::Disabled_Disable, Enums::Disabled_Names ),
      _enumHighCapacity     ("High Capacity",       Enums::Disabled_Disable, Enums::Disabled_Names ),
      _u8GainIndex          ("Gain Index",          0,    0,    5),
      _u16ReadoutSpeedIndex ("Readout Speed",       0,    0,    5),
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
      pList.insert(&_enumFanMode);
      pList.insert(&_enumBaselineClamp);
      pList.insert(&_enumHighCapacity);
      pList.insert(&_u8GainIndex);
      pList.insert(&_u16ReadoutSpeedIndex);
      pList.insert(&_u16ExposureEventCode);
      pList.insert(&_u32NumDelayShots);
    }

    int pull(void* from) {
      AndorConfigType& tc = *new(from) AndorConfigType;
      _uWidth               .value = tc.width   ();
      _uHeight              .value = tc.height  ();
      _uOrgX                .value = tc.orgX    ();
      _uOrgY                .value = tc.orgY    ();
      _uBinX                .value = tc.binX    ();
      _uBinY                .value = tc.binY    ();
      _f32ExposureTime      .value = tc.exposureTime();
      _f32CoolingTemp       .value = tc.coolingTemp ();
      _enumFanMode          .value = (AndorConfigType::EnumFanMode) tc.fanMode();
      _enumBaselineClamp    .value = (Enums::Disabled) tc.baselineClamp();
      _enumHighCapacity     .value = (Enums::Disabled) tc.highCapacity();
      _u8GainIndex          .value = tc.gainIndex();
      _u16ReadoutSpeedIndex .value = tc.readoutSpeedIndex();
      _u16ExposureEventCode .value = tc.exposureEventCode();
      _u32NumDelayShots     .value = tc.numDelayShots();
      return tc._sizeof();
    }

    int push(void* to) {
      AndorConfigType& tc = *new(to) AndorConfigType(
        _uWidth               .value,
        _uHeight              .value,
        _uOrgX                .value,
        _uOrgY                .value,
        _uBinX                .value,
        _uBinY                .value,
        _f32ExposureTime      .value,
        _f32CoolingTemp       .value,
        _enumFanMode          .value,
        _enumBaselineClamp    .value,
        _enumHighCapacity     .value,
        _u8GainIndex          .value,
        _u16ReadoutSpeedIndex .value,
        _u16ExposureEventCode .value,
        _u32NumDelayShots     .value
      );
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(AndorConfigType);
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
    Enumerated<AndorConfigType::EnumFanMode>  _enumFanMode;
    Enumerated<Enums::Disabled>               _enumBaselineClamp;
    Enumerated<Enums::Disabled>               _enumHighCapacity;
    NumericInt<uint8_t>     _u8GainIndex;        
    NumericInt<uint16_t>    _u16ReadoutSpeedIndex;        
    NumericInt<uint16_t>    _u16ExposureEventCode;
    NumericInt<uint32_t>    _u32NumDelayShots;
  };
  
  const char* AndorConfig::Private_Data::lsEnumFanMode[] = { "Full", "Low", "Off", "Off during Acq", NULL};    
  
};

using namespace Pds_ConfigDb;

AndorConfig::AndorConfig() : 
  Serializer("andor_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  AndorConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  AndorConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  AndorConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

