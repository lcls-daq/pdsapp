#include "EvrConfig_V1.hh"

#include "pdsapp/config/EvrPulseConfig.hh"
#include "pdsapp/config/EvrOutputMap.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsdata/evr/ConfigV1.hh"
#include "pdsdata/evr/PulseConfig.hh"
#include "pdsdata/evr/OutputMap.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {
  class EvrConfig_V1::Private_Data {
    enum { MaxPulses=32 };
    enum { MaxOutputs=10 };
  public:
    Private_Data() :
      _npulses ("Number of Pulses" , 0, 0, MaxPulses),
      _noutputs("Number of Outputs", 0, 0, MaxOutputs),
      _pulseSet("Pulse Definition"  , _pulseArgs , _npulses ),
      _outputSet("Output Definition", _outputArgs, _noutputs)
    {
      for(unsigned k=0; k<MaxPulses; k++)
	_pulses[k].insert(_pulseArgs[k]);
      for(unsigned k=0; k<MaxOutputs; k++)
	_outputs[k].insert(_outputArgs[k]);
    }

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_npulses);
      pList.insert(&_noutputs);
      pList.insert(&_pulseSet);
      pList.insert(&_outputSet);
    }

    int pull(void* from) {
      const Pds::EvrData::ConfigV1& tc = *new(from) Pds::EvrData::ConfigV1;
      _npulses.value  = tc.npulses();
      _noutputs.value = tc.noutputs();
      for(unsigned k=0; k<tc.npulses(); k++)
	_pulses[k].pull(const_cast<Pds::EvrData::PulseConfig*>(&tc.pulse(k)));
      for(unsigned k=0; k<tc.noutputs(); k++)
	_outputs[k].pull(const_cast<Pds::EvrData::OutputMap*>(&tc.output_map(k)));
      return tc.size();
    }

    int push(void* to) {
      Pds::EvrData::PulseConfig* pc = new Pds::EvrData::PulseConfig[_npulses.value];
      for(unsigned k=0; k<_npulses.value; k++)
	_pulses[k].push(&pc[k]);
      Pds::EvrData::OutputMap* mo = new Pds::EvrData::OutputMap[_noutputs.value];
      for(unsigned k=0; k<_noutputs.value; k++)
	_outputs[k].push(&mo[k]);
      Pds::EvrData::ConfigV1& tc = *new(to) Pds::EvrData::ConfigV1(_npulses.value,
							  pc,
							  _noutputs.value,
							  mo);
      
      delete[] pc;
      delete[] mo;
      return tc.size();
    }

    int dataSize() const {
      return sizeof(Pds::EvrData::ConfigV1) + 
	_npulses .value*sizeof(Pds::EvrData::PulseConfig) + 
	_noutputs.value*sizeof(Pds::EvrData::OutputMap);
    }
  public:
    NumericInt<unsigned>    _npulses;
    NumericInt<unsigned>    _noutputs;
    EvrPulseConfig _pulses [MaxPulses];
    EvrOutputMap   _outputs[MaxOutputs];
    Pds::LinkedList<Parameter> _pulseArgs [MaxPulses];
    Pds::LinkedList<Parameter> _outputArgs[MaxOutputs];
    ParameterSet   _pulseSet;
    ParameterSet   _outputSet;
  };
};


EvrConfig_V1::EvrConfig_V1() : 
  Serializer("Evr_Config_V1"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  EvrConfig_V1::readParameters (void* from) {
  return _private_data->pull(from);
}

int  EvrConfig_V1::writeParameters(void* to) {
  return _private_data->push(to);
}

int  EvrConfig_V1::dataSize() const {
  return _private_data->dataSize();
}
