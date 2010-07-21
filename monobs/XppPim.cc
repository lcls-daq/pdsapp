#include "pdsapp/monobs/XppPim.hh"
#include "pdsapp/monobs/PVWriter.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/ipimb/DataV1.hh"

#include <string.h>
#include <stdio.h>

using namespace PdsCas;

XppPim::XppPim(const char* pvbase, int detid, float* basev) :
  Handler(Pds::DetInfo(-1,
		       Pds::DetInfo::Detector(detid), 1, Pds::DetInfo::Ipimb, 0),
	  Pds::TypeId::Id_IpimbData,
	  Pds::TypeId::Id_IpimbConfig),
  _initialized(false)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
  for(unsigned i=0; i<NDIODES; i++)
    _base[i] = basev[i];
}

XppPim::~XppPim()
{
  if (_initialized) {
    for(unsigned i=0; i<NDIODES; i++)
      delete _valu_writer[i];
  }
}

void XppPim::initialize()
{
  char buff[64];
  for(unsigned i=0; i<NDIODES; i++) {
    sprintf(buff,"%s:CH%d",_pvName,i);
    _valu_writer[i] = new PVWriter(buff);
  }
  _initialized = true;
}


void XppPim::_configure(const void* payload, const Pds::ClockTime& t) {}


void XppPim::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  const Pds::Ipimb::DataV1& data = *reinterpret_cast<const Pds::Ipimb::DataV1*>(payload);
  double v0 = _base[0] - data.channel0Volts();
  double v1 = _base[1] - data.channel1Volts();
  double v2 = _base[2] - data.channel2Volts();
  double v3 = _base[3] - data.channel3Volts();

  *reinterpret_cast<double*>(_valu_writer[0]->data()) = v0;
  *reinterpret_cast<double*>(_valu_writer[1]->data()) = v1;
  *reinterpret_cast<double*>(_valu_writer[2]->data()) = v2;
  *reinterpret_cast<double*>(_valu_writer[3]->data()) = v3;

  for(unsigned i=0; i<NDIODES; i++) {
    _valu_writer[i]->put();
  }
}

void XppPim::_damaged  () {}

void XppPim::update_pv() {}
