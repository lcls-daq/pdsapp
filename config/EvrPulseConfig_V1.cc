#include "EvrPulseConfig_V1.hh"

#include "pdsdata/evr/PulseConfig.hh"

using namespace Pds_ConfigDb;

static const double EvrPeriod = 1./119.e6;

EvrPulseConfig_V1::EvrPulseConfig_V1() :
  _pulse      ("Pulse"  ,  0, 0, 31),
  _trigger    ("Trigger", -1, -1, 31),
  _set        ("Set"    , -1, -1, 31),
  _clear      ("Clear"  , -1, -1, 31),
  _polarity   ("Polarity"   , Enums::Pos   , Enums::Polarity_Names),
  _map_set_ena("Map_Set"    , Enums::Enable, Enums::Enabled_Names),
  _map_rst_ena("Map_Reset"  , Enums::Enable, Enums::Enabled_Names),
  _map_trg_ena("Map_Trigger", Enums::Enable, Enums::Enabled_Names),
  _prescale   ("Prescale", 1, 1, 0x7fffffff),
  _delay      ("Delay [sec]"   , 0, 0, 0x7fffffff, Scaled, EvrPeriod),
  _width      ("Width [sec]"   , 0, 0, 0x7fffffff, Scaled, EvrPeriod)
{}
   
void EvrPulseConfig_V1::insert(Pds::LinkedList<Parameter>& pList) {
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

bool EvrPulseConfig_V1::pull(void* from) {
  Pds::EvrData::PulseConfig& tc = *new(from) Pds::EvrData::PulseConfig;
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

int EvrPulseConfig_V1::push(void* to) {
  Pds::EvrData::PulseConfig& tc = *new(to) Pds::EvrData::PulseConfig(_pulse.value,
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
