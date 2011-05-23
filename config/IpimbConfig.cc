#include "IpimbConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/IpimbConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {
  
  class IpimbConfig::Private_Data {
  public:
    typedef IpimbConfigType::CapacitorValue CapacitorValue;
    Private_Data() :      
      //      _triggerCounter("Foo ", 1, 1, 4),
      //      _serialID("Foo ", 1, 1, 4),
      _chargeAmpRange0("Channel 0 feedback capacitor (pF) ", IpimbConfigType::c_1pF, IpimbConfigType::cap_range),
      _chargeAmpRange1("Channel 1 feedback capacitor (pF) ", IpimbConfigType::c_1pF, IpimbConfigType::cap_range),
      _chargeAmpRange2("Channel 2 feedback capacitor (pF) ", IpimbConfigType::c_1pF, IpimbConfigType::cap_range),
      _chargeAmpRange3("Channel 3 feedback capacitor (pF) ", IpimbConfigType::c_1pF, IpimbConfigType::cap_range),
      //      _calibrationRange("Calibration cap (pF) ", 1, 1, 10000),
      _resetLength("Acquisition window (ns) ", 1000000, 1, 0xfffff),
      _resetDelay("Reset delay (ns) ", 0xfff, 0, 0xfff),
      _chargeAmpRefVoltage("Reference voltage ", 2.0, 0.0, 12.0),
      //      _calibrationVoltage("Calibration voltage ", 0., 0., 0.),
      _diodeBias("Diode bias voltage ", 0., 0., 200.),
      //      _status("Foo ", 1, 1, 4),
      //      _errors("Foo ", 1, 1, 4),
      //      _calStrobeLength("Calibration strobe length ", 0, 0, 0),
      _trigDelay("Sampling delay (ns) ", 191000, 0, 0x7fff8),
      _chargeAmpRange(0) {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      //      pList.insert(&_triggerCounter);
      //      pList.insert(&_serialID);
      pList.insert(&_chargeAmpRange0);
      pList.insert(&_chargeAmpRange1);
      pList.insert(&_chargeAmpRange2);
      pList.insert(&_chargeAmpRange3);
      //      pList.insert(&_calibrationRange);
      pList.insert(&_resetLength);
      pList.insert(&_resetDelay);
      pList.insert(&_chargeAmpRefVoltage);
      //      pList.insert(&_calibrationVoltage);
      pList.insert(&_diodeBias);
      //      pList.insert(&_status);
      //      pList.insert(&_errors);
      //      pList.insert(&_calStrobeLength);
      pList.insert(&_trigDelay);
    }

    int pull(void* from) { // pull "from xtc"
      IpimbConfigType& ipimbConf = *new(from) IpimbConfigType;
      //      _triggerCounter.value = ipimbConf.triggerCounter();
      //      _serialID.value = ipimbConf.serialID();
      //      _chargeAmpRange.value = ipimbConf.chargeAmpRange();
      _chargeAmpRange = ipimbConf.chargeAmpRange();
      _chargeAmpRange0.value = (CapacitorValue)(std::min(_chargeAmpRange & 0xf, (int)IpimbConfigType::expert));
      _chargeAmpRange1.value = (CapacitorValue)(std::min((_chargeAmpRange>>4) & 0xf, (int)IpimbConfigType::expert));
      _chargeAmpRange2.value = (CapacitorValue)(std::min((_chargeAmpRange>>8) & 0xf, (int)IpimbConfigType::expert));
      _chargeAmpRange3.value = (CapacitorValue)(std::min((_chargeAmpRange>>12) & 0xf, (int)IpimbConfigType::expert));
      //      _calibrationRange.value = ipimbConf.calibrationRange();
      _resetLength.value = ipimbConf.resetLength();
      _resetDelay.value = ipimbConf.resetDelay();
      _chargeAmpRefVoltage.value = ipimbConf.chargeAmpRefVoltage();
      //      _calibrationVoltage.value = ipimbConf.calibrationVoltage();
      _diodeBias.value = ipimbConf.diodeBias();
      //      _status.value = ipimbConf.status();
      //      _errors.value = ipimbConf.errors();
      //      _calStrobeLength.value = ipimbConf.calStrobeLength();
      _trigDelay.value  = ipimbConf.trigDelay();
      _trigPsDelay = ipimbConf.trigPsDelay();
      _adcDelay = ipimbConf.adcDelay();
      return sizeof(IpimbConfigType);
    }

    int push(void* to) {
      *new(to) IpimbConfigType(_chargeAmpRange,
                               0, //_calibrationRange.value,
                               _resetLength.value,
                               _resetDelay.value,
                               _chargeAmpRefVoltage.value,
                               0., //_calibrationVoltage.value,
                               _diodeBias.value,
                               0, //_calStrobeLength.value,
                               _trigDelay.value,
			       0,
			       0
                               );
      return sizeof(IpimbConfigType);
    }

    int dataSize() const { return sizeof(IpimbConfigType); }
    void updateChargeAmpRange() {
      _chargeAmpRange = (_chargeAmpRange0.value&0xf) + 
	((_chargeAmpRange1.value&0xf)<<4) + 
	((_chargeAmpRange2.value&0xf)<<8) +  
	((_chargeAmpRange3.value&0xf)<<12);
    }

  public:
    //    typedef IpimbConfigType::CapacitorValue CapacitorValue;
    //    NumericInt<uint64_t> _triggerCounter;
    //    NumericInt<uint64_t> _serialID;
    Enumerated<CapacitorValue> _chargeAmpRange0;
    Enumerated<CapacitorValue> _chargeAmpRange1;
    Enumerated<CapacitorValue> _chargeAmpRange2;
    Enumerated<CapacitorValue> _chargeAmpRange3;
    //    NumericInt<uint16_t> _chargeAmpRange;
    //    NumericInt<uint16_t> _calibrationRange;
    NumericInt<uint32_t> _resetLength;
    NumericInt<uint16_t> _resetDelay;
    NumericFloat<float> _chargeAmpRefVoltage;
    //    NumericFloat<float> _calibrationVoltage;
    NumericFloat<float> _diodeBias;
    //    NumericInt<uint16_t> _status;
    //    NumericInt<uint16_t> _errors;
    //    NumericInt<uint16_t> _calStrobeLength;
    NumericInt<uint32_t> _trigDelay;
    uint16_t _chargeAmpRange;
    uint32_t _trigPsDelay;
    uint32_t _adcDelay;
  };
};

IpimbConfig::IpimbConfig() : 
  Serializer("Ipimb_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int IpimbConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  IpimbConfig::writeParameters(void* to) {
  _private_data->updateChargeAmpRange();
  return _private_data->push(to);
}

int  IpimbConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"
