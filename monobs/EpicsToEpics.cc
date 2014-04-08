#include "pdsapp/monobs/EpicsToEpics.hh"

#include "pdsdata/psddl/epics.ddl.h"

#include "pds/epicstools/PVWriter.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <string.h>
#include <stdio.h>

using namespace PdsCas;

using Pds_Epics::PVWriter;

EpicsToEpics::EpicsToEpics(const DetInfo& info,
                           const char* pvIn,
                           const char* pvOut) :
  Handler(info,
	  Pds::TypeId::Id_Epics,
	  Pds::TypeId::Id_Epics),
  _pvIn  (pvIn),
  _pvOut (pvOut),
  _index (-1),
  _initialized(false)
{
}

EpicsToEpics::~EpicsToEpics()
{
  if (_initialized) {
    delete _valu_writer;
  }
}

void EpicsToEpics::initialize()
{
  _valu_writer = new PVWriter(_pvOut.c_str());
  _initialized = true;
}


void EpicsToEpics::_configure(const void* payload, const Pds::ClockTime& t) 
{
  const Pds::Epics::EpicsPvHeader& pvData = 
    *reinterpret_cast<const Pds::Epics::EpicsPvHeader*>(payload);

  if (pvData.dbrType() >= DBR_CTRL_SHORT &&
      pvData.dbrType() <= DBR_CTRL_DOUBLE) {
    const Pds::Epics::EpicsPvCtrlHeader& ctrl = static_cast<const Pds::Epics::EpicsPvCtrlHeader&>(pvData);

    if (std::string(ctrl.pvName())==_pvIn) {
      _index = ctrl.pvId();
      printf("EpicsToEpics found %s [%s]\n",_pvIn.c_str(),_pvOut.c_str());
    }
  }
  else {
  }
}


#define CASETOVAL(timetype,pdstimetype,valtype) case timetype: {        \
    const pdstimetype& p = static_cast<const pdstimetype&>(pvData);     \
    const valtype* v = p.data().data();                                 \
    *reinterpret_cast<valtype*>(_valu_writer->data()) = *v;             \
    break; }

void EpicsToEpics::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  const Pds::Epics::EpicsPvHeader& pvData = *reinterpret_cast<const Pds::Epics::EpicsPvHeader*>(payload);
  
  if (pvData.pvId() == _index &&
      pvData.dbrType() < DBR_CTRL_STRING) {
    switch(pvData.dbrType()) {
      CASETOVAL(DBR_TIME_SHORT ,Pds::Epics::EpicsPvTimeShort , int16_t )
      CASETOVAL(DBR_TIME_FLOAT ,Pds::Epics::EpicsPvTimeFloat , float   )
      CASETOVAL(DBR_TIME_ENUM  ,Pds::Epics::EpicsPvTimeEnum  , uint16_t)
      CASETOVAL(DBR_TIME_LONG  ,Pds::Epics::EpicsPvTimeLong  , int32_t )
      CASETOVAL(DBR_TIME_DOUBLE,Pds::Epics::EpicsPvTimeDouble, double  )
      break;
    }
    _valu_writer->put();
  }
}

void EpicsToEpics::_damaged  () {}

void EpicsToEpics::update_pv() {}
