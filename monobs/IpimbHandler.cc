#include "pdsapp/monobs/IpimbHandler.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/ipimb/DataV1.hh"

#include <string.h>
#include <stdio.h>

using namespace PdsCas;

IpimbHandler::IpimbHandler(const char* pvbase, const Pds::DetInfo& info) :
  Handler(info,
	  Pds::TypeId::Id_IpimbData,
	  Pds::TypeId::Id_IpimbConfig),
  _initialized(false)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
}

IpimbHandler::~IpimbHandler()
{
  if (_initialized) {
    for(unsigned i=0; i<NDIODES; i++)
      delete _valu_writer[i];
  }
}

void IpimbHandler::initialize()
{
  char buff[64];
  for(unsigned i=0; i<NDIODES; i++) {
    sprintf(buff,"%s:VAL%d.VAL",_pvName,i+1);
    _valu_writer[i] = new PVWriter(buff);
  }

  _initialized = true;
}


void IpimbHandler::_configure(const void* payload, const Pds::ClockTime& t) 
{
}


void IpimbHandler::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  const Pds::Ipimb::DataV1& data =
    *reinterpret_cast<const Pds::Ipimb::DataV1*>(payload);

  *reinterpret_cast<double*>(_valu_writer[0]->data()) = data.channel0Volts();
  *reinterpret_cast<double*>(_valu_writer[1]->data()) = data.channel1Volts();
  *reinterpret_cast<double*>(_valu_writer[2]->data()) = data.channel2Volts();
  *reinterpret_cast<double*>(_valu_writer[3]->data()) = data.channel3Volts();

  for(unsigned i=0; i<NDIODES; i++) {
    _valu_writer[i]->put();
  }
}

void IpimbHandler::_damaged  () {}

void IpimbHandler::update_pv() {}
