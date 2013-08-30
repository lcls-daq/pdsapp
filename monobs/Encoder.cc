#include "pdsapp/monobs/Encoder.hh"

#include "pds/epicstools/PVWriter.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/encoder.ddl.h"

#include <string.h>
#include <stdio.h>

using Pds_Epics::PVWriter;

using namespace PdsCas;

PdsCas::Encoder::Encoder(const char* pvbase, int detid) :
  Handler(Pds::DetInfo(0,
                       Pds::DetInfo::Detector(detid),0,
                       Pds::DetInfo::Encoder,0),
	  Pds::TypeId::Id_EncoderData,
	  Pds::TypeId::Id_EncoderConfig),
  _initialized(false)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
}

PdsCas::Encoder::~Encoder()
{
  if (_initialized) {
    delete _valu_writer;
  }
}

void PdsCas::Encoder::initialize()
{
  char buff[64];
  sprintf(buff,"%s:ENCODER",_pvName);
  _valu_writer = new PVWriter(buff);

  _initialized = true;
}


void PdsCas::Encoder::_configure(const void* payload, const Pds::ClockTime& t) 
{
}


void PdsCas::Encoder::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  const Pds::Encoder::DataV2& data =
    *reinterpret_cast<const Pds::Encoder::DataV2*>(payload);

  unsigned nch = _valu_writer->data_size()/sizeof(double);
  if (nch > NCHANNELS) nch = NCHANNELS;
  for(unsigned i=0; i<nch; i++)
    reinterpret_cast<double*>(_valu_writer->data())[i] = data.encoder_count()[i];

  _valu_writer->put();
}

void PdsCas::Encoder::_damaged  () {}

void PdsCas::Encoder::update_pv() {}
