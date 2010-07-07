#include "EvrPulseConfig.hh"

#include "pds/config/EvrConfigType.hh"

using namespace Pds_ConfigDb;

static const double EvrPeriod = 1./119.e6;

EvrPulseConfig::EvrPulseConfig() :
  _polarity   ("Polarity"   , Enums::Pos   , Enums::Polarity_Names),
  _prescale   ("Prescale", 1, 1, 0x7fffffff),
  _delay      ("Delay [sec]"   , 0, 0, 0x7fffffff, Scaled, EvrPeriod),
  _width      ("Width [sec]"   , 0, 0, 0x7fffffff, Scaled, EvrPeriod)
{}

void EvrPulseConfig::id(unsigned pid) { _pulse=pid; }

void EvrPulseConfig::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_polarity);
  pList.insert(&_prescale);
  pList.insert(&_delay);
  pList.insert(&_width);
}

bool EvrPulseConfig::pull(void* from) {
  const EvrConfigType::PulseType& tc = *(const EvrConfigType::PulseType*) from;
  if (_pulse != tc.pulseId())
    printf("Read pulse id %d.  Returning pulse id %d.\n",tc.pulseId(),_pulse);
  _polarity.value = tc.polarity() ? Enums::Pos : Enums::Neg ;
  _prescale.value = tc.prescale();
  _delay.value = tc.delay();
  _width.value = tc.width();
  return true;
}

int EvrPulseConfig::push(void* to) {
  EvrConfigType::PulseType& tc = *new(to) EvrConfigType::PulseType(
    _pulse,
    ((_polarity.value==Enums::Pos)? 0: 1),
    _prescale.value,
    _delay.value,
    _width.value);
  return sizeof(tc);
}
