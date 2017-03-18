#define __STDC_LIMIT_MACROS

#include "JungfrauConfig_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsdata/psddl/jungfrau.ddl.h"

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


using namespace Pds_ConfigDb;

class Pds_ConfigDb::JungfrauConfig_V1::Private_Data {
  static const char*  lsEnumGainMode[];
  static const char*  lsEnumSpeedMode[];
 public:
   Private_Data();
  ~Private_Data();

   void insert( Pds::LinkedList<Parameter>& pList );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(Pds::Jungfrau::ConfigV1); }

  NumericInt<uint32_t> _numberOfModules;
  NumericInt<uint32_t> _numberOfRowsPerModule;
  NumericInt<uint32_t> _numberOfColumnsPerModule;
  NumericInt<uint32_t> _biasVoltage;
  Enumerated<Pds::Jungfrau::ConfigV1::GainMode> _gainMode;
  Enumerated<Pds::Jungfrau::ConfigV1::SpeedMode> _speedMode;
  NumericFloat<double> _triggerDelay;
  NumericFloat<double> _exposureTime;
};

const char* Pds_ConfigDb::JungfrauConfig_V1::Private_Data::lsEnumGainMode[] = { "Normal", "FixedGain1", "FixedGain2", "ForcedGain1", "ForcedGain2", "HighGain0", NULL};
const char* Pds_ConfigDb::JungfrauConfig_V1::Private_Data::lsEnumSpeedMode[] = { "Quarter", "Half", NULL};

Pds_ConfigDb::JungfrauConfig_V1::Private_Data::Private_Data() :
  _numberOfModules          ("Number of Modules",         1,    1,      4),
  _numberOfRowsPerModule    ("Number of Rows",            512,  1,      512),
  _numberOfColumnsPerModule ("Number of Columns",         1024, 1,      1024),
  _biasVoltage              ("Bias Voltage",              200,  0,      500),
  _gainMode                 ("Gain Mode",                 Pds::Jungfrau::ConfigV1::Normal, lsEnumGainMode),
  _speedMode                ("Speed Mode",                Pds::Jungfrau::ConfigV1::Quarter, lsEnumSpeedMode),
  _triggerDelay             ("Trigger Delay (s)",         0.000238, 0., 10.),
  _exposureTime             ("Exposure Time (s)",         0.000010, 0., 120.)
{}

Pds_ConfigDb::JungfrauConfig_V1::Private_Data::~Private_Data() 
{}

void Pds_ConfigDb::JungfrauConfig_V1::Private_Data::insert( Pds::LinkedList<Parameter>& pList )
{
  pList.insert( &_numberOfModules );
  pList.insert( &_numberOfRowsPerModule );
  pList.insert( &_numberOfColumnsPerModule );
  pList.insert( &_biasVoltage );
  pList.insert( &_gainMode );
  pList.insert( &_speedMode );
  pList.insert( &_triggerDelay );
  pList.insert( &_exposureTime );
}

int Pds_ConfigDb::JungfrauConfig_V1::Private_Data::pull( void* from )
{
  Pds::Jungfrau::ConfigV1& cfg = * new (from) Pds::Jungfrau::ConfigV1;
  _numberOfModules.value          = cfg.numberOfModules();
  _numberOfRowsPerModule.value    = cfg.numberOfRowsPerModule();
  _numberOfColumnsPerModule.value = cfg.numberOfColumnsPerModule();
  _biasVoltage.value              = cfg.biasVoltage();
  _gainMode.value                 = (Pds::Jungfrau::ConfigV1::GainMode) cfg.gainMode();
  _speedMode.value                = (Pds::Jungfrau::ConfigV1::SpeedMode) cfg.speedMode();
  _triggerDelay.value             = cfg.triggerDelay();
  _exposureTime.value             = cfg.exposureTime();
  return sizeof(Pds::Jungfrau::ConfigV1);
}
  
int Pds_ConfigDb::JungfrauConfig_V1::Private_Data::push(void* to)
{
  new (to) Pds::Jungfrau::ConfigV1(
    _numberOfModules.value,
    _numberOfRowsPerModule.value,
    _numberOfColumnsPerModule.value,
    _biasVoltage.value,
    _gainMode.value,
    _speedMode.value,
    _triggerDelay.value,
    _exposureTime.value
  );
  
  return sizeof(Pds::Jungfrau::ConfigV1);
}
 
Pds_ConfigDb::JungfrauConfig_V1::JungfrauConfig_V1() :
  Serializer("JungfrauConfig_V1"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int Pds_ConfigDb::JungfrauConfig_V1::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::JungfrauConfig_V1::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::JungfrauConfig_V1::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"
