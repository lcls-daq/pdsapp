#include "EvrConfig_V2.hh"

#include "pdsapp/config/EvrPulseConfig_V1.hh"
#include "pdsapp/config/EvrOutputMap_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsdata/psddl/evr.ddl.h"

#include <new>

using namespace Pds_ConfigDb::EvrConfig_V2;

namespace Pds_ConfigDb {
  namespace EvrConfig_V2 {
    static const char* beam_range[] = { "Off", "On", NULL };
    static const char* rate_range[] = { "120Hz/Full", "60Hz", "30Hz", "10Hz", "5Hz", "1Hz", "0.5Hz", NULL };

    class EvrConfig::Private_Data {
      enum { MaxPulses=32 };
      enum { MaxOutputs=10 };
    public:
      Private_Data() :
        _bc      ("Beam"   , Pds::EvrData::ConfigV2::On, beam_range),
        _rc      ("Rate"   , Pds::EvrData::ConfigV2::r30Hz, rate_range),
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
        const Pds::EvrData::ConfigV2& tc = *reinterpret_cast<Pds::EvrData::ConfigV2*>(from);
        _bc      .value  = tc.beam();
        _rc      .value  = tc.rate();
        _npulses .value  = tc.npulses();
        _noutputs.value  = tc.noutputs();
        for(unsigned k=0; k<tc.npulses(); k++)
          _pulses[k].pull(tc.pulses()[k]);
        for(unsigned k=0; k<tc.noutputs(); k++)
          _outputs[k].pull(tc.output_maps()[k]);
        return tc._sizeof();
      }

      int push(void* to) {
        Pds::EvrData::PulseConfig* pc = new Pds::EvrData::PulseConfig[_npulses.value];
        for(unsigned k=0; k<_npulses.value; k++)
          _pulses[k].push(&pc[k]);
        Pds::EvrData::OutputMap* mo = new Pds::EvrData::OutputMap[_noutputs.value];
        for(unsigned k=0; k<_noutputs.value; k++)
          _outputs[k].push(&mo[k]);

        unsigned opcode = _rc.value == Pds::EvrData::ConfigV2::Single ?
          Pds::EvrData::ConfigV2::singleShot :
          Pds::EvrData::ConfigV2::baseRate + _rc.value + _bc.value*Pds::EvrData::ConfigV2::beamOn;

        Pds::EvrData::ConfigV2& tc = *new(to) Pds::EvrData::ConfigV2(opcode,
                                                                     _npulses.value,
                                                                     _noutputs.value,
                                                                     pc, mo);
      
        delete[] pc;
        delete[] mo;
        return tc._sizeof();
      }

      int dataSize() const {
        return sizeof(Pds::EvrData::ConfigV2) + 
          _npulses .value*sizeof(Pds::EvrData::PulseConfig) + 
          _noutputs.value*sizeof(Pds::EvrData::OutputMap);
      }
    public:
      Enumerated<Pds::EvrData::ConfigV2::BeamCode> _bc;
      Enumerated<Pds::EvrData::ConfigV2::RateCode> _rc;
      NumericInt<unsigned>    _npulses;
      NumericInt<unsigned>    _noutputs;
      EvrPulseConfig_V1 _pulses [MaxPulses];
      EvrOutputMap_V1   _outputs[MaxOutputs];
      Pds::LinkedList<Parameter> _pulseArgs [MaxPulses];
      Pds::LinkedList<Parameter> _outputArgs[MaxOutputs];
      ParameterSet   _pulseSet;
      ParameterSet   _outputSet;
    };
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
