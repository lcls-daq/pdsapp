#include "EvrEventCode.hh"

#include "pds/config/EvrConfigType.hh"

using namespace Pds_ConfigDb;

//static const double EvrPeriod = 1./119.e6;

EvrEventCode::EvrEventCode() :
  _code         ("Event Code",    0, 0, 255),
  _desc         ("Desc"   ,   "", EvrConfigType::EventCodeType::DescSize),
  _isReadout    ("Readout",       Enums::False   , Enums::Bool_Names),
  _isCommand    ("Command",       Enums::False   , Enums::Bool_Names),
  _isLatch      ("Latch"     ,    Enums::False   , Enums::Bool_Names),
  _reportDelay  ("Report Delay",  0, 0, 0xffff),
  _reportWidth  ("Report Width/Release",  1, 1, 0xffff),
  _maskTrigger  ("Trigger Mask",  0, 0, 0x03ff, Hex),
  _maskSet      ("Set     Mask",  0, 0, 0x03ff, Hex),
  _maskClear    ("Clear   Mask",  0, 0, 0x03ff, Hex)
{}

void EvrEventCode::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_code);
  pList.insert(&_desc);
  pList.insert(&_isReadout);
  pList.insert(&_isCommand);
  pList.insert(&_isLatch);
  pList.insert(&_reportDelay);
  pList.insert(&_reportWidth);  
  pList.insert(&_maskTrigger);
  pList.insert(&_maskSet);
  pList.insert(&_maskClear);
}

bool EvrEventCode::pull(void* from) {
  const EvrConfigType::EventCodeType& eventCode = *(const EvrConfigType::EventCodeType*) from;
  
  _code        .value = eventCode.code();
  strncpy(_desc.value, eventCode.desc(),EvrConfigType::EventCodeType::DescSize);
  _isReadout   .value = ( eventCode.isReadout()     ? Enums::True : Enums::False );
  _isCommand   .value = ( eventCode.isCommand()     ? Enums::True : Enums::False );
  _isLatch     .value = ( eventCode.isLatch()       ? Enums::True : Enums::False );
  _reportDelay .value = eventCode.reportDelay();
  _reportWidth .value = eventCode.reportWidth();
  _maskTrigger .value = eventCode.maskTrigger();
  _maskSet     .value = eventCode.maskSet();
  _maskClear   .value = eventCode.maskClear();
  
  return true;
}

int EvrEventCode::push(void* to) {
  EvrConfigType::EventCodeType& evntCode = *new (to) EvrConfigType::EventCodeType(
    (uint8_t) _code.value,
    _desc        .value,
    _isReadout   .value == Enums::True,
    _isCommand   .value == Enums::True,
    _isLatch     .value == Enums::True,
    _reportDelay .value,
    _reportWidth .value,
    _maskTrigger .value,
    _maskSet     .value,
    _maskClear   .value );

  return sizeof(evntCode);
}
