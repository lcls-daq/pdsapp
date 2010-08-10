#include "pdsapp/monobs/XppIpm.hh"
#include "pdsapp/monobs/PVWriter.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/lusi/IpmFexV1.hh"

#include <string.h>
#include <stdio.h>

using namespace PdsCas;

XppIpm::XppIpm(const char* pvbase, int detid) :
  Handler(Pds::DetInfo(-1,
		       Pds::DetInfo::Detector(detid), 1, Pds::DetInfo::Ipimb, 0),
	  Pds::TypeId::Id_IpmFex,
	  Pds::TypeId::Id_IpmFexConfig),
  _initialized(false)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
}

XppIpm::~XppIpm()
{
  if (_initialized) {
    for(unsigned i=0; i<NDIODES; i++)
      delete _valu_writer[i];
    delete _sum_writer;
    delete _xpos_writer;
    delete _ypos_writer;
  }
}

void XppIpm::initialize()
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


void XppIpm::_configure(const void* payload, const Pds::ClockTime& t) 
{
}


void XppIpm::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  const Pds::Lusi::IpmFexV1& data =
    *reinterpret_cast<const Pds::Lusi::IpmFexV1*>(payload);

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

void XppIpm::_damaged  () {}

void XppIpm::update_pv() {}
