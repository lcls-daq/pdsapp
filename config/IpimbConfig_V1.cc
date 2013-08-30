#include "IpimbConfig_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pdsdata/psddl/ipimb.ddl.h"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {
  
  enum CapacitorValue {c_1pF, c_100pF, c_10nF};
  static const char* cap_range[] = { "1 pF", "100 pF", "10 nF", NULL };

  class IpimbConfig_V1::Private_Data {
  public:
    Private_Data() :
      //      _triggerCounter("Foo ", 1, 1, 4),
      //      _serialID("Foo ", 1, 1, 4),
      _chargeAmpRange0("Channel 0 feedback capacitor (pF) ", c_1pF, cap_range),
      _chargeAmpRange1("Channel 1 feedback capacitor (pF) ", c_1pF, cap_range),
      _chargeAmpRange2("Channel 2 feedback capacitor (pF) ", c_1pF, cap_range),
      _chargeAmpRange3("Channel 3 feedback capacitor (pF) ", c_1pF, cap_range),
      //      _calibrationRange("Calibration cap (pF) ", 1, 1, 10000),
      _resetLength("Acquisition window (ns) ", 1000000, 1, 0xfffff),
      _resetDelay("Reset delay (ns) ", 0xfff, 0, 0xfff),
      _chargeAmpRefVoltage("Reference voltage ", 2.0, 0.0, 12.0),
      //      _calibrationVoltage("Calibration voltage ", 0., 0., 0.),
      _diodeBias("Diode bias voltage ", 0., 0., 200.),
      //      _status("Foo ", 1, 1, 4),
      //      _errors("Foo ", 1, 1, 4),
      //      _calStrobeLength("Calibration strobe length ", 0, 0, 0),
      _trigDelay("Sampling delay (ns) ", 89000, 0, 0x7fff8),
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
      Pds::Ipimb::ConfigV1& ipimbConf = *new(from) Pds::Ipimb::ConfigV1;
      //      _triggerCounter.value = ipimbConf.triggerCounter();
      //      _serialID.value = ipimbConf.serialID();
      //      _chargeAmpRange.value = ipimbConf.chargeAmpRange();
      _chargeAmpRange = ipimbConf.chargeAmpRange();
      for (int chan=0; chan<4; chan++) {
	unsigned i=0;
	while(((_chargeAmpRange>>(2*chan))&0x3) != i && ++i<c_10nF);
	if (chan==0) {_chargeAmpRange0.value = CapacitorValue(i);}
	if (chan==1) {_chargeAmpRange1.value = CapacitorValue(i);}
	if (chan==2) {_chargeAmpRange2.value = CapacitorValue(i);}
	if (chan==3) {_chargeAmpRange3.value = CapacitorValue(i);}
      }
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
      return sizeof(Pds::Ipimb::ConfigV1);
    }

    int push(void* to) {
      *new(to) Pds::Ipimb::ConfigV1(0, 0,
                                    _chargeAmpRange,
                                    0, //_calibrationRange.value,
                                    _resetLength.value,
                                    _resetDelay.value,
                                    _chargeAmpRefVoltage.value,
                                    0., //_calibrationVoltage.value,
                                    _diodeBias.value,
                                    0, 0, // status, errors
                                    0, //_calStrobeLength.value,
                                    _trigDelay.value
                                    );
      return sizeof(Pds::Ipimb::ConfigV1);
    }

    int dataSize() const { return sizeof(Pds::Ipimb::ConfigV1); }
    void updateChargeAmpRange() {
      _chargeAmpRange = (_chargeAmpRange0.value&0x3) + 
	((_chargeAmpRange1.value&0x3)<<2) + 
	((_chargeAmpRange2.value&0x3)<<4) +  
	((_chargeAmpRange3.value&0x3)<<6);
    }

  public:
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
  };
};

IpimbConfig_V1::IpimbConfig_V1() : 
  Serializer("Ipimb_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int IpimbConfig_V1::readParameters (void* from) {
  return _private_data->pull(from);
}

int  IpimbConfig_V1::writeParameters(void* to) {
  _private_data->updateChargeAmpRange();
  return _private_data->push(to);
}

int  IpimbConfig_V1::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"
