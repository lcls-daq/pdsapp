#include "EvrConfig.hh"

#include "EvrPulseConfig.hh"
#include "EvrOutputMap.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/EvrConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {

  static const char* beam_range[] = { "Off", "On", NULL };
  static const char* rate_range[] = { "120Hz/Full", "60Hz", "30Hz", "10Hz", "5Hz", "1Hz", "0.5Hz", NULL };

  class EvrConfig::Private_Data {
    enum { MaxPulses=32 };
    enum { MaxOutputs=10 };
  public:
    Private_Data() :
      _bc      ("Beam"   , EvrConfigType::On, beam_range),
      _rc      ("Rate"   , EvrConfigType::r30Hz, rate_range),
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
      pList.insert(&_bc);
      pList.insert(&_rc);
      pList.insert(&_npulses);
      pList.insert(&_noutputs);
      pList.insert(&_pulseSet);
      pList.insert(&_outputSet);
    }

    int pull(void* from) {
      const EvrConfigType& tc = *new(from) EvrConfigType;
      _bc      .value  = tc.beam();
      _rc      .value  = tc.rate();
      _npulses .value  = tc.npulses();
      _noutputs.value  = tc.noutputs();
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
      EvrConfigType& tc = *new(to) EvrConfigType(_bc.value,
						 _rc.value,
						 _npulses.value,
						 pc,
						 _noutputs.value,
						 mo);
      
      delete[] pc;
      delete[] mo;
      return tc.size();
    }

    int dataSize() const {
      return sizeof(EvrConfigType) + 
	_npulses .value*sizeof(Pds::EvrData::PulseConfig) + 
	_noutputs.value*sizeof(Pds::EvrData::OutputMap);
    }
  public:
    Enumerated<EvrConfigType::BeamCode> _bc;
    Enumerated<EvrConfigType::RateCode> _rc;
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


EvrConfig::EvrConfig() : 
  Serializer("Evr_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  EvrConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  EvrConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  EvrConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<Pds::EvrData::ConfigV2::BeamCode>;
template class Enumerated<Pds::EvrData::ConfigV2::RateCode>;
