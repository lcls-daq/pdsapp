#include "IpimbConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/IpimbConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {

  //  enum IpimbVFullScale {V0_1,V0_2,V0_5,V1,V2,V5};
  //  static const double fullscale_value[] = {0.1,0.2,0.5,1.0,2.0,5.0};
  //  static const char*  fullscale_range[] = {"0.1","0.2","0.5","1","2","5",NULL};

  //  static const char* coupling_range[] = {"GND","DC","AC","DC50ohm","AC50ohm",NULL};
  //  static const char* bandwidth_range[]= {"None","25MHz","700MHz","200MHz","20MHz","35MHz",NULL};

  class IpimbConfig::Private_Data {
  public:
    Private_Data() :
      _triggerCounter("Foo ", 1, 1, 4),
      _serialID("Foo ", 1, 1, 4),
      _chargeAmpRange("Foo ", 1, 1, 4),
      //      _calibrationRange("Foo ", 1, 1, 4),
      _resetLength("Foo ", 1, 1, 4),
      _resetDelay("Foo ", 1, 1, 4),
      _chargeAmpRefVoltage("Foo ", 1, 1, 4),
      //      _calibrationVoltage("Foo ", 1, 1, 4),
      _diodeBias("Foo ", 1, 1, 4),
      _srStatusResets("Foo ", 1, 1, 4),
      _errors("Foo ", 1, 1, 4),
      _calStrobeLength("Foo ", 1, 1, 4),
      _trigDelay("Foo ", 1, 1, 4) {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_triggerCounter);
      pList.insert(&_serialID);
      pList.insert(&_chargeAmpRange);
      //      pList.insert(&_calibrationRange);
      pList.insert(&_resetLength);
      pList.insert(&_resetDelay);
      pList.insert(&_chargeAmpRefVoltage);
      //      pList.insert(&_calibrationVoltage);
      pList.insert(&_diodeBias);
      pList.insert(&_srStatusResets);
      pList.insert(&_errors);
      pList.insert(&_calStrobeLength);
      pList.insert(&_trigDelay);
    }

    int pull(void* from) { // pull "from xtc"
      IpimbConfigType& ipimbConf = *new(from) IpimbConfigType;
      _triggerCounter.value = ipimbConf.triggerCounter();
      _serialID.value = ipimbConf.serialID();
      _chargeAmpRange.value = ipimbConf.chargeAmpRange();
      //      _calibrationRange.value = ipimbConf.calibrationRange();
      _resetLength.value = ipimbConf.resetLength();
      _resetDelay.value = ipimbConf.resetDelay();
      _chargeAmpRefVoltage.value = ipimbConf.chargeAmpRefVoltage();
      //      _calibrationVoltage.value = ipimbConf.calibrationVoltage();
      _diodeBias.value = ipimbConf.diodeBias();
      _srStatusResets.value = ipimbConf.srStatusResets();
      _errors.value = ipimbConf.errors();
      _calStrobeLength.value = ipimbConf.calStrobeLength();
      _trigDelay.value  = ipimbConf.trigDelay();
      return sizeof(IpimbConfigType);
    }

    int push(void* to) { // push "to xtc"
      *new(to) IpimbConfigType(_triggerCounter.value,
			       _serialID.value,
			       _chargeAmpRange.value,
			       //			       _calibrationRange.value,
			       _resetLength.value,
			       _resetDelay.value,
			       _chargeAmpRefVoltage.value,
			       //			       _calibrationVoltage.value,
			       _diodeBias.value,
			       _srStatusResets.value,
			       _errors.value,
			       _calStrobeLength.value,
			       _trigDelay.value
			       );
      return sizeof(IpimbConfigType);
    }

    int dataSize() const { return sizeof(IpimbConfigType); }
  public:
    NumericInt<uint64_t> _triggerCounter;
    NumericInt<uint64_t> _serialID;
    NumericInt<uint16_t> _chargeAmpRange;
    //    NumericInt<uint16_t> _calibrationRange;
    NumericInt<uint32_t> _resetLength;
    NumericInt<uint16_t> _resetDelay;
    NumericFloat<float> _chargeAmpRefVoltage;
    //    NumericInt<uint16_t> _calibrationVoltage;
    NumericFloat<float> _diodeBias;
    NumericInt<uint16_t> _srStatusResets;
    NumericInt<uint16_t> _errors;
    NumericInt<uint16_t> _calStrobeLength;
    NumericInt<uint16_t> _trigDelay;
    //    BitCount _numChan;
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
  return _private_data->push(to);
}

int  IpimbConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"
