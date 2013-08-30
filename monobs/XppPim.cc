#include "pdsapp/monobs/XppPim.hh"

#include "pds/epicstools/PVWriter.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/lusi.ddl.h"

#include <string.h>
#include <stdio.h>

using namespace PdsCas;

XppPim::XppPim(const char* pvbase, int detid) :
  Handler(Pds::DetInfo(-1,
		       Pds::DetInfo::Detector(detid), 1, Pds::DetInfo::Ipimb, 0),
	  Pds::TypeId::Id_DiodeFex,
	  Pds::TypeId::Id_DiodeFexConfig),
  _initialized(false)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
}

XppPim::~XppPim()
{
  if (_initialized) {
    delete _valu_writer;
  }
}

void XppPim::initialize()
{
  char buff[64];
  sprintf(buff,"%s:CH0",_pvName);
  _valu_writer = new PVWriter(buff);
  _initialized = true;
}


void XppPim::_configure(const void* payload, const Pds::ClockTime& t) {}


void XppPim::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  *reinterpret_cast<double*>(_valu_writer->data()) = 
    reinterpret_cast<const Pds::Lusi::DiodeFexV1*>(payload)->value();
  _valu_writer->put();
}

void XppPim::_damaged  () {}

void XppPim::update_pv() {}
