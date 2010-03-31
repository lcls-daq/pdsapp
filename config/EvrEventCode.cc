#include "EvrEventCode.hh"

#include "pds/config/EvrConfigType.hh"

using namespace Pds_ConfigDb;

static const double EvrPeriod = 1./119.e6;

EvrEventCode::EvrEventCode() :
  _code         ("Event Code",    0, 0, 255),
  _isReadout    ("Readout",       Enums::False   , Enums::Bool_Names),
  _isTerminator ("Terminator",    Enums::False   , Enums::Bool_Names),
  _maskTrigger  ("Trigger Mask",  0, 0, 0x03ff, Hex),
  _maskSet      ("Set     Mask",      0, 0, 0x03ff, Hex),
  _maskClear    ("Clear   Mask"   , 0, 0, 0x03ff, Hex)
{}

void EvrEventCode::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_code);
  pList.insert(&_isReadout);
  pList.insert(&_isTerminator);
  pList.insert(&_maskTrigger);
  pList.insert(&_maskSet);
  pList.insert(&_maskClear);
}

bool EvrEventCode::pull(void* from) {
  const EvrConfigType::EventCodeType& eventCode = *(const EvrConfigType::EventCodeType*) from;
  
  _code.value         = eventCode.code();
  _isReadout.value    = ( eventCode.isReadout()     ? Enums::True : Enums::False );
  _isTerminator.value = ( eventCode.isTerminator()  ? Enums::True : Enums::False );
  _maskTrigger.value  = eventCode.maskTrigger();
  _maskSet.value      = eventCode.maskSet();
  _maskClear.value    = eventCode.maskClear();
  
  return true;
}

int EvrEventCode::push(void* to) {
  EvrConfigType::EventCodeType& evntCode = *new (to) EvrConfigType::EventCodeType(
    (uint8_t) _code.value,
    _isReadout.value    == Enums::True,
    _isTerminator.value == Enums::True,
    _maskTrigger.value,
    _maskSet.value,
    _maskClear.value );

  return sizeof(evntCode);
}
