#include "ConfigTC_EvrConfig.hh"

#include "pdsapp/config/ConfigTC_Parameters.hh"
#include "pdsapp/config/ConfigTC_ParameterSet.hh"
#include "pdsdata/evr/ConfigV1.hh"

#include <new>

using namespace ConfigGui;

namespace ConfigGui {

  class EvrPulseConfig {
  public:
    EvrPulseConfig() :
    _pulse      ("Pulse"  ,  0, 0, 31),
    _trigger    ("Trigger", -1, -1, 31),
    _set        ("Set"    , -1, -1, 31),
    _clear      ("Clear"  , -1, -1, 31),
    _polarity   ("Polarity"   , Enums::Pos   , Enums::Polarity_Names),
    _map_set_ena("Map_Set"    , Enums::Enable, Enums::Enabled_Names),
    _map_rst_ena("Map_Reset"  , Enums::Enable, Enums::Enabled_Names),
    _map_trg_ena("Map_Trigger", Enums::Enable, Enums::Enabled_Names),
    _prescale   ("Prescale", 1, 1, 0x7fffffff),
    _delay      ("Delay"   , 0, 0, 0x7fffffff),
    _width      ("Width"   , 0, 0, 0x7fffffff)
    {}
   
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_pulse);
      pList.insert(&_trigger);
      pList.insert(&_set);
      pList.insert(&_clear);
      pList.insert(&_polarity);
      pList.insert(&_map_set_ena);
      pList.insert(&_map_rst_ena);
      pList.insert(&_map_trg_ena);
      pList.insert(&_prescale);
      pList.insert(&_delay);
      pList.insert(&_width);
    }

    bool pull(void* from) {
      EvrData::PulseConfig& tc = *new(from) EvrData::PulseConfig;
      _pulse.value = tc.pulse();
      _trigger.value = tc.trigger();
      _set.value = tc.set();
      _clear.value = tc.clear();
      _polarity.value = tc.polarity() ? Enums::Pos : Enums::Neg ;
      _map_set_ena.value = tc.map_set_enable    () ? Enums::Enable : Enums::Disable;
      _map_rst_ena.value = tc.map_reset_enable  () ? Enums::Enable : Enums::Disable;
      _map_trg_ena.value = tc.map_trigger_enable() ? Enums::Enable : Enums::Disable;
      _prescale.value = tc.prescale();
      _delay.value = tc.delay();
      _width.value = tc.width();
      return true;
    }

    int push(void* to) {
      EvrData::PulseConfig& tc = *new(to) EvrData::PulseConfig(_pulse.value,
						       _trigger.value,
						       _set.value,
						       _clear.value,
						       _polarity.value==Enums::Pos,
						       _map_set_ena.value==Enums::Enable,
						       _map_rst_ena.value==Enums::Enable,
						       _map_trg_ena.value==Enums::Enable,
						       _prescale.value,
						       _delay.value,
						       _width.value);
      return sizeof(tc);
    }
  private:
    NumericInt<unsigned>    _pulse;
    NumericInt<int>         _trigger;
    NumericInt<int>         _set;
    NumericInt<int>         _clear;
    Enumerated<Enums::Polarity>    _polarity;
    Enumerated<Enums::Enabled>     _map_set_ena;
    Enumerated<Enums::Enabled>     _map_rst_ena;
    Enumerated<Enums::Enabled>     _map_trg_ena;
    NumericInt<unsigned>    _prescale;
    NumericInt<unsigned>    _delay;
    NumericInt<unsigned>    _width;
  };
    
  static const char* source_range[] = { "Pulse",
					"DBus",
					"Prescaler",
					"Force_High",
					"Force_Low",
					NULL };
  static const char* conn_range[] = { "Front Panel",
				      "Univ IO",
				      NULL };

  class EvrOutputMap {
  public:
    EvrOutputMap() :
    _source    ("Source"   , EvrData::OutputMap::Pulse, source_range),
    _source_id ("Source id", 0, 0, 9),
    _conn      ("Conn"     , EvrData::OutputMap::FrontPanel, conn_range),
    _conn_id   ("Conn id"  , 0, 0, 9)
    {}
   
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_source);
      pList.insert(&_source_id);
      pList.insert(&_conn);
      pList.insert(&_conn_id);
    }

    bool pull(void* from) {
      EvrData::OutputMap& tc = *new(from) EvrData::OutputMap;
      _source   .value = tc.source();
      _source_id.value = tc.source_id();
      _conn     .value = tc.conn();
      _conn_id  .value = tc.conn_id();
      return true;
    }

    int push(void* to) {
      EvrData::OutputMap& tc = *new(to) EvrData::OutputMap(_source.value,
						   _source_id.value,
						   _conn.value,
						   _conn_id.value);
      return sizeof(tc);
    }
  private:
    Enumerated<EvrData::OutputMap::Source> _source;
    NumericInt<unsigned>               _source_id;
    Enumerated<EvrData::OutputMap::Conn> _conn;
    NumericInt<unsigned>             _conn_id;
  };
    
  class EvrConfig::Private_Data {
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

    bool pull(void* from) {
      const EvrData::ConfigV1& tc = *new(from) EvrData::ConfigV1;
      _npulses.value  = tc.npulses();
      _noutputs.value = tc.noutputs();
      for(unsigned k=0; k<tc.npulses(); k++)
	_pulses[k].pull(const_cast<EvrData::PulseConfig*>(&tc.pulse(k)));
      for(unsigned k=0; k<tc.noutputs(); k++)
	_outputs[k].pull(const_cast<EvrData::OutputMap*>(&tc.output_map(k)));
      return true;
    }

    int push(void* to) {
      EvrData::PulseConfig* pc = new EvrData::PulseConfig[_npulses.value];
      for(unsigned k=0; k<_npulses.value; k++)
	_pulses[k].push(&pc[k]);
      EvrData::OutputMap* mo = new EvrData::OutputMap[_noutputs.value];
      for(unsigned k=0; k<_noutputs.value; k++)
	_outputs[k].push(&mo[k]);
      EvrData::ConfigV1& tc = *new(to) EvrData::ConfigV1(_npulses.value,
						 pc,
						 _noutputs.value,
						 mo);
      
      delete[] pc;
      delete[] mo;
      return tc.size();
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


EvrConfig::EvrConfig() : 
  Serializer("Evr_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

bool EvrConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  EvrConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

#include "ConfigTC_Parameters.icc"

template class Enumerated<EvrData::OutputMap::Source>;
template class Enumerated<EvrData::OutputMap::Conn>;
