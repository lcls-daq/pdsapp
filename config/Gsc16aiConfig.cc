// Gsc16aiConfig.cc

#include "pdsapp/config/Gsc16aiConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/Gsc16aiConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  // these must end in NULL
  static const char* voltageRange_to_name[] = { "+/- 10 V", "+/- 5 V", "+/- 2.5 V", NULL };
  static const char* channelSelect_to_name[] =
      { "channel 0",     "channels 0-1" , "channels 0-2",  "channels 0-3",  "channels 0-4",
        "channels 0-5",  "channels 0-6",  "channels 0-7",  "channels 0-8",  "channels 0-9",
        "channels 0-10", "channels 0-11", "channels 0-12", "channels 0-13", "channels 0-14",
        "channels 0-15", NULL };

//class Gsc16aiConfig::Private_Data {
  class Gsc16aiConfig::Private_Data {
  public:
    Private_Data() :
      _voltageRange   ("Voltage Range", Pds::Gsc16ai::ConfigV1::VoltageRange_10V, voltageRange_to_name),
      _channelSelect  ("Input Channels", 0, channelSelect_to_name)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_voltageRange);
      pList.insert(&_channelSelect);
    }

    int pull(void* from) {
      Gsc16aiConfigType& tc = *new(from) Gsc16aiConfigType;
      _voltageRange.value = (Pds::Gsc16ai::ConfigV1::VoltageRange)tc.voltageRange();
      _channelSelect.value = (Pds::Gsc16ai::ConfigV1::VoltageRange)tc.lastChan();
      return tc.size();
    }

    int push(void* to) {
      Gsc16aiConfigType& tc = *new(to) Gsc16aiConfigType(
        _voltageRange.value,
        Pds::Gsc16ai::ConfigV1::LowestChannel,
        _channelSelect.value
      );
      return tc.size();
    }

    int dataSize() const {
      return sizeof(Gsc16aiConfigType);
    }

  public:
    Enumerated<Pds::Gsc16ai::ConfigV1::VoltageRange> _voltageRange;
    Enumerated<int> _channelSelect;
  };
};

using namespace Pds_ConfigDb;

Gsc16aiConfig::Gsc16aiConfig() : 
  Serializer("Gsc16ai_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  Gsc16aiConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  Gsc16aiConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  Gsc16aiConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

