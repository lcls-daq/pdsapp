#include "pdsapp/monobs/BldIpm.hh"
#include "pdsapp/monobs/PVWriter.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/bld/bldData.hh"
#include "pdsdata/lusi/IpmFexV1.hh"

#include <string.h>
#include <stdio.h>

using namespace PdsCas;

BldIpm::BldIpm(const char* pvbase, int bldid) :
  Handler(Pds::BldInfo(-1,BldInfo::Type(bldid)),
	  Pds::TypeId::Id_SharedIpimb,
	  Pds::TypeId::Id_SharedIpimb),
  _initialized(false)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
}

BldIpm::~BldIpm()
{
  if (_initialized) {
    for(unsigned i=0; i<NDIODES; i++)
      delete _valu_writer[i];
    delete _sum_writer;
    delete _xpos_writer;
    delete _ypos_writer;
  }
}

void BldIpm::initialize()
{
  char buff[64];
  for(unsigned i=0; i<NDIODES; i++) {
    sprintf(buff,"%s:CH%d.VAL",_pvName,i);
    _valu_writer[i] = new PVWriter(buff);
  }
  sprintf(buff,"%s:SUM.VAL",_pvName);
  _sum_writer = new PVWriter(buff);
  sprintf(buff,"%s:XPOS.VAL",_pvName);
  _xpos_writer = new PVWriter(buff);
  sprintf(buff,"%s:YPOS.VAL",_pvName);
  _ypos_writer = new PVWriter(buff);

  _initialized = true;
}


void BldIpm::_configure(const void* payload, const Pds::ClockTime& t) 
{
}


void BldIpm::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  const Pds::BldDataIpimb& p =
    *reinterpret_cast<const Pds::BldDataIpimb*>(payload);

  const Pds::Lusi::IpmFexV1& data = p.ipmFexData;

  for(unsigned i=0; i<NDIODES; i++)
    *reinterpret_cast<double*>(_valu_writer[i]->data()) = data.channel[i];
  *reinterpret_cast<double*>(_sum_writer ->data()) = data.sum;
  *reinterpret_cast<double*>(_xpos_writer->data()) = data.xpos;
  *reinterpret_cast<double*>(_ypos_writer->data()) = data.ypos;

  for(unsigned i=0; i<NDIODES; i++) {
    _valu_writer[i]->put();
  }
  _sum_writer ->put();
  _xpos_writer->put();
  _ypos_writer->put();
}

void BldIpm::_damaged  () {}

void BldIpm::update_pv() {}
