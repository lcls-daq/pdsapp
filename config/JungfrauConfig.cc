#define __STDC_LIMIT_MACROS

#include "JungfrauConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/JungfrauConfigType.hh"

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


using namespace Pds_ConfigDb;

class Pds_ConfigDb::JungfrauConfig::Private_Data {
  static const char*  lsEnumGainMode[];
  static const char*  lsEnumSpeedMode[];
 public:
   Private_Data();
  ~Private_Data();

   void insert( Pds::LinkedList<Parameter>& pList );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(JungfrauConfigType); }

  NumericInt<uint32_t> _numberOfModules;
  NumericInt<uint32_t> _numberOfRowsPerModule;
  NumericInt<uint32_t> _numberOfColumnsPerModule;
  NumericInt<uint32_t> _biasVoltage;
  Enumerated<JungfrauConfigType::GainMode> _gainMode;
  Enumerated<JungfrauConfigType::SpeedMode> _speedMode;
  NumericFloat<double> _triggerDelay;
  NumericFloat<double> _exposureTime;
};

const char* Pds_ConfigDb::JungfrauConfig::Private_Data::lsEnumGainMode[] = { "Normal", "FixedGain1", "FixedGain2", "ForcedGain1", "ForcedGain2", "HighGain0", NULL};
const char* Pds_ConfigDb::JungfrauConfig::Private_Data::lsEnumSpeedMode[] = { "Quarter", "Half", NULL};

Pds_ConfigDb::JungfrauConfig::Private_Data::Private_Data() :
  _numberOfModules          ("Number of Modules",         1,    1,      4),
  _numberOfRowsPerModule    ("Number of Rows",            512,  1,      512),
  _numberOfColumnsPerModule ("Number of Columns",         1024, 1,      1024),
  _biasVoltage              ("Bias Voltage",              200,  0,      500),
  _gainMode                 ("Gain Mode",                 JungfrauConfigType::Normal, lsEnumGainMode),
  _speedMode                ("Speed Mode",                JungfrauConfigType::Quarter, lsEnumSpeedMode),
  _triggerDelay             ("Trigger Delay (s)",         0.000238, 0., 10.),
  _exposureTime             ("Exposure Time (s)",         0.000010, 0., 120.)
{}

Pds_ConfigDb::JungfrauConfig::Private_Data::~Private_Data() 
{}

void Pds_ConfigDb::JungfrauConfig::Private_Data::insert( Pds::LinkedList<Parameter>& pList )
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

int Pds_ConfigDb::JungfrauConfig::Private_Data::pull( void* from )
{
  JungfrauConfigType& cfg = * new (from) JungfrauConfigType;
  _numberOfModules.value          = cfg.numberOfModules();
  _numberOfRowsPerModule.value    = cfg.numberOfRowsPerModule();
  _numberOfColumnsPerModule.value = cfg.numberOfColumnsPerModule();
  _biasVoltage.value              = cfg.biasVoltage();
  _gainMode.value                 = (JungfrauConfigType::GainMode) cfg.gainMode();
  _speedMode.value                = (JungfrauConfigType::SpeedMode) cfg.speedMode();
  _triggerDelay.value             = cfg.triggerDelay();
  _exposureTime.value             = cfg.exposureTime();
  return sizeof(JungfrauConfigType);
}
  
int Pds_ConfigDb::JungfrauConfig::Private_Data::push(void* to)
{
  new (to) JungfrauConfigType(
    _numberOfModules.value,
    _numberOfRowsPerModule.value,
    _numberOfColumnsPerModule.value,
    _biasVoltage.value,
    _gainMode.value,
    _speedMode.value,
    _triggerDelay.value,
    _exposureTime.value
  );
  
  return sizeof(JungfrauConfigType);
}
 
Pds_ConfigDb::JungfrauConfig::JungfrauConfig() :
  Serializer("JungfrauConfig"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int Pds_ConfigDb::JungfrauConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::JungfrauConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::JungfrauConfig::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"
