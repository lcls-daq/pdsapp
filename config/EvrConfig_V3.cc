#include "pdsdata/evr/ConfigV3.hh"

#include "EvrEventCodeV3.hh"
#include "EvrPulseConfig.hh"
#include "EvrOutputMap.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "EvrConfig_V3.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb
{
  class EvrConfig_V3::Private_Data
  {
    enum
    { MaxEventCodes = 32 };
    enum
    { MaxPulses = 32 };
    enum
    { MaxOutputs = 10 };
  public:
      Private_Data() :
        _neventcodes("Number of Event Codes", 0, 0, MaxEventCodes),      
        _npulses("Number of Pulses", 0, 0, MaxPulses),
        _noutputs("Number of Outputs", 0, 0, MaxOutputs),
        _eventcodeSet("Event Code Definition", _eventcodeArgs, _neventcodes),
        _pulseSet("Pulse Definition", _pulseArgs, _npulses),
        _outputSet("Output Definition", _outputArgs, _noutputs)
    {
      for (unsigned k = 0; k < MaxEventCodes; k++)
      {
        _eventcodes[k].insert(_eventcodeArgs[k]);
      }
      
      for (unsigned k = 0; k < MaxPulses; k++)
      {
        _pulses[k].id(k);
        _pulses[k].insert(_pulseArgs[k]);
      }
      
      for (unsigned k = 0; k < MaxOutputs; k++)
        _outputs[k].insert(_outputArgs[k]);
    }

    void insert(Pds::LinkedList < Parameter > &pList)
    {
      pList.insert(&_neventcodes);
      pList.insert(&_eventcodeSet);
      pList.insert(&_npulses);
      pList.insert(&_pulseSet);
      pList.insert(&_noutputs);
      pList.insert(&_outputSet);
    }

    int pull(void *from)
    {
      const Pds::EvrData::ConfigV3& tc = *(const Pds::EvrData::ConfigV3*) from;
      _neventcodes.value = tc.neventcodes();
      _npulses.value = tc.npulses();
      _noutputs.value = tc.noutputs();
      
      for (unsigned k = 0; k < tc.neventcodes(); k++)
        _eventcodes[k].pull(const_cast <
          Pds::EvrData::ConfigV3::EventCodeType * >(&tc.eventcode(k)));
      for (unsigned k = 0; k < tc.npulses(); k++)
        _pulses[k].pull(const_cast <
          Pds::EvrData::ConfigV3::PulseType * >(&tc.pulse(k)));
      for (unsigned k = 0; k < tc.noutputs(); k++)
        _outputs[k].pull(const_cast <
          Pds::EvrData::ConfigV3::OutputMapType * >(&tc.output_map(k)));
      return tc.size();
    }

    int push(void *to)
    {
      Pds::EvrData::ConfigV3::EventCodeType * eventcodes = 
        new Pds::EvrData::ConfigV3::EventCodeType[_neventcodes.value];
      
      for (unsigned k = 0; k < _neventcodes.value; k++)
        _eventcodes[k].push(&eventcodes[k]);

      Pds::EvrData::ConfigV3::PulseType * pc = 
        new Pds::EvrData::ConfigV3::PulseType[_npulses.value];
      
      for (unsigned k = 0; k < _npulses.value; k++)
        _pulses[k].push(&pc[k]);
        
      Pds::EvrData::ConfigV3::OutputMapType * mo =
        new Pds::EvrData::ConfigV3::OutputMapType[_noutputs.value];
        
      for (unsigned k = 0; k < _noutputs.value; k++)
        _outputs[k].push(&mo[k]);
        
      Pds::EvrData::ConfigV3 & tc = *new (to) Pds::EvrData::ConfigV3(
              _neventcodes.value, eventcodes,
              _npulses.value,     pc, 
              _noutputs.value,    mo);

      delete[] eventcodes;
      delete[] pc;
      delete[] mo;
      return tc.size();
    }

    int dataSize() const
    {
      return sizeof(Pds::EvrData::ConfigV3) + 
        _neventcodes.value * sizeof(Pds::EvrData::ConfigV3::EventCodeType) +
        _npulses.value * sizeof(Pds::EvrData::ConfigV3::PulseType) +
        _noutputs.value * sizeof(Pds::EvrData::ConfigV3::OutputMapType);
    }
  public:
    NumericInt < unsigned >                 _neventcodes;      
    NumericInt < unsigned >                 _npulses;
    NumericInt < unsigned >                 _noutputs;
    EvrEventCodeV3                          _eventcodes[MaxEventCodes];
    EvrPulseConfig                          _pulses[MaxPulses];
    EvrOutputMap                            _outputs[MaxOutputs];
    Pds::LinkedList < Parameter >           _eventcodeArgs[MaxEventCodes];
    Pds::LinkedList < Parameter >           _pulseArgs[MaxPulses];
    Pds::LinkedList < Parameter >           _outputArgs[MaxOutputs];
    ParameterSet                            _eventcodeSet;
    ParameterSet                            _pulseSet;
    ParameterSet                            _outputSet;
  };
};


EvrConfig_V3::EvrConfig_V3():
Serializer("Evr_Config"), _private_data(new Private_Data)
{
  _private_data->insert(pList);
}

int EvrConfig_V3::readParameters(void *from)
{
  return _private_data->pull(from);
}

int EvrConfig_V3::writeParameters(void *to)
{
  return _private_data->push(to);
}

int EvrConfig_V3::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"
