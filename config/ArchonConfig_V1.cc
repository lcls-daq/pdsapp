#define __STDC_LIMIT_MACROS

#include "ArchonConfig_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsdata/psddl/archon.ddl.h"

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


using namespace Pds_ConfigDb;

class Pds_ConfigDb::ArchonConfig_V1::Private_Data {
  static const char*  lsEnumReadoutMode[];
 public:
   Private_Data();
  ~Private_Data();

   void insert( Pds::LinkedList<Parameter>& pList );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(Pds::Archon::ConfigV1); }

  Enumerated<Pds::Archon::ConfigV1::ReadoutMode>  _enumReadoutMode;
  NumericInt<uint16_t>  _sweepCount;
  NumericInt<uint32_t>  _integrationTime;
  NumericInt<uint32_t>  _nonIntegrationTime;
  //-------- Image Formatting parameters --------
  NumericInt<uint32_t>  _preSkipPixels;
  NumericInt<uint32_t>  _pixels;
  NumericInt<uint32_t>  _postSkipPixels;
  NumericInt<uint32_t>  _overscanPixels;
  NumericInt<uint16_t>  _preSkipLines;
  NumericInt<uint16_t>  _lines;
  NumericInt<uint16_t>  _postSkipLines;
  NumericInt<uint16_t>  _overScanLines;
  NumericInt<uint16_t>  _horizontalBinning;
  NumericInt<uint16_t>  _verticalBinning;
  //-------- Timing Parameters --------
  NumericInt<uint16_t>  _rgh;
  NumericInt<uint16_t>  _rgl;
NumericInt<uint16_t>  _shp;
  NumericInt<uint16_t>  _shd;
  NumericInt<uint16_t>  _st;
  NumericInt<uint16_t>  _stm1;
  NumericInt<uint16_t>  _at;
  NumericInt<uint16_t>  _dwell1;
  NumericInt<uint16_t>  _dwell2;
  //-------- Constants - Clock Level --------
  NumericInt<int16_t>  _rgHigh;
  NumericInt<int16_t>  _rgLow;
  NumericInt<int16_t>  _sHigh;
  NumericInt<int16_t>  _sLow;
  NumericInt<int16_t>  _aHigh;
  NumericInt<int16_t>  _aLow;
  //-------- Constants - Slew Rates --------
  NumericInt<int16_t>  _rgSlew;
  NumericInt<int16_t>  _sSlew;
  NumericInt<int16_t>  _aSlew;

  TextParameter*        _conf;
};

const char* Pds_ConfigDb::ArchonConfig_V1::Private_Data::lsEnumReadoutMode[] = { "Single", "Continuous", "Triggered", NULL};

Pds_ConfigDb::ArchonConfig_V1::Private_Data::Private_Data() :
  _enumReadoutMode    ("Readout Mode",          Pds::Archon::ConfigV1::Single, lsEnumReadoutMode),
  _sweepCount         ("Sweep Count",               0,    0,      65535),
  _integrationTime    ("Integration Time (ms)",     0,    0,      0xFFFFFFFF),
  _nonIntegrationTime ("Non-Integration Time (ms)", 100,  0,      0xFFFFFFFF),
  _preSkipPixels      ("Pre Skip Pixels",           0,    0,      0xFFFFFFFF),
  _pixels             ("Pixel Count",               600,  0,      0xFFFFFFFF),
  _postSkipPixels     ("Post Skip Pixels",          0,    0,      0xFFFFFFFF),
  _overscanPixels     ("Overscan Pixels",           0,    0,      0xFFFFFFFF),
  _preSkipLines       ("Pre Skip Lines",            0,    0,      65535),
  _lines              ("Line Count",                2050, 0,      65535),
  _postSkipLines      ("Prot Skip Lines",           0,    0,      65535),
  _overScanLines      ("Overscan Lines",            0,    0,      65535),
  _horizontalBinning  ("Horizontal Binning",        0,    0,      65535),
  _verticalBinning    ("Vertical Binning",          1,    0,      65535),
  _rgh                ("RGH",                       39,   0,      65535),
  _rgl                ("RGL",                       9,    0,      65535),
  _shp                ("SHP",                       393,  0,      65535),
  _shd                ("SHD",                       402,  0,      65535),
  _st                 ("ST",                        60,   0,      65535),
  _stm1               ("STM1",                      29,   0,      65535),
  _at                 ("AT",                        1000, 0,      65535),
  _dwell1             ("DWELL1",                    400,  0,      65535),
  _dwell2             ("DWELL2",                    200,  0,      65535),
  _rgHigh             ("RG HIGH",                   10,   -32768, 32767),
  _rgLow              ("RG LOW",                    -2,   -32768, 32767),
  _sHigh              ("S HIGH",                    5,    -32768, 32767),
  _sLow               ("S LOW",                     -5,   -32768, 32767),
  _aHigh              ("A HIGH",                    2,    -32768, 32767),
  _aLow               ("A LOW",                     -9,   -32768, 32767),
  _rgSlew             ("RG SLEW",                   1000, -32768, 32767),
  _sSlew              ("S SLEW",                    1000, -32768, 32767),
  _aSlew              ("A SLEW",                    1000, -32768, 32767)
{
  _conf = new TextParameter("Configuration File", "", Pds::Archon::ConfigV1::FILENAME_CHAR_MAX);
}

Pds_ConfigDb::ArchonConfig_V1::Private_Data::~Private_Data() 
{
  delete    _conf;
}

void Pds_ConfigDb::ArchonConfig_V1::Private_Data::insert( Pds::LinkedList<Parameter>& pList )
{
  pList.insert( &_enumReadoutMode );
  pList.insert( &_sweepCount );
  pList.insert( &_integrationTime );
  pList.insert( &_nonIntegrationTime );
  pList.insert( &_preSkipPixels );
  pList.insert( &_pixels );
  pList.insert( &_postSkipPixels );
  pList.insert( &_overscanPixels );
  pList.insert( &_preSkipLines );
  pList.insert( &_lines );
  pList.insert( &_postSkipLines );
  pList.insert( &_overScanLines );
  pList.insert( &_horizontalBinning );
  pList.insert( &_verticalBinning );
  pList.insert( &_rgh );
  pList.insert( &_rgl );
  pList.insert( &_shp );
  pList.insert( &_shd );
  pList.insert( &_st );
  pList.insert( &_stm1 );
  pList.insert( &_at );
  pList.insert( &_dwell1 );
  pList.insert( &_dwell2 );
  pList.insert( &_rgHigh );
  pList.insert( &_rgLow );
  pList.insert( &_sHigh );
  pList.insert( &_sLow );
  pList.insert( &_aHigh );
  pList.insert( &_aLow );
  pList.insert( &_rgSlew );
  pList.insert( &_sSlew );
  pList.insert( &_aSlew );
  pList.insert( _conf );
}

int Pds_ConfigDb::ArchonConfig_V1::Private_Data::pull( void* from )
{
  Pds::Archon::ConfigV1& cfg = * new (from) Pds::Archon::ConfigV1;
  _enumReadoutMode.value    = (Pds::Archon::ConfigV1::ReadoutMode) cfg.readoutMode();
  _sweepCount.value         = cfg.sweepCount();
  _integrationTime.value    = cfg.integrationTime();
  _nonIntegrationTime.value = cfg.nonIntegrationTime();
  _preSkipPixels.value      = cfg.preSkipPixels();
  _pixels.value             = cfg.pixels();
  _postSkipPixels.value     = cfg.postSkipPixels();
  _overscanPixels.value     = cfg.overscanPixels();
  _preSkipLines.value       = cfg.preSkipLines();
  _lines.value              = cfg.lines();
  _postSkipLines.value      = cfg.postSkipLines();
  _overScanLines.value      = cfg.overScanLines();
  _horizontalBinning.value  = cfg.horizontalBinning();
  _verticalBinning.value    = cfg.verticalBinning();
  _rgh.value                = cfg.rgh();
  _rgl.value                = cfg.rgl();
  _shp.value                = cfg.shp();
  _shd.value                = cfg.shd();
  _st.value                 = cfg.st();
  _stm1.value               = cfg.stm1();
  _at.value                 = cfg.at();
  _dwell1.value             = cfg.dwell1();
  _dwell2.value             = cfg.dwell2();
  _rgHigh.value             = cfg.rgHigh();
  _rgLow.value              = cfg.rgLow();
  _sHigh.value              = cfg.sHigh();
  _sLow.value               = cfg.sLow();
  _aHigh.value              = cfg.aHigh();
  _aLow.value               = cfg.aLow();
  _rgSlew.value             = cfg.rgSlew();
  _sSlew.value              = cfg.sSlew();
  _aSlew.value              = cfg.aSlew();
  strncpy(_conf->value, cfg.config(), Pds::Archon::ConfigV1::FILENAME_CHAR_MAX);
  return sizeof(Pds::Archon::ConfigV1);
}
  
int Pds_ConfigDb::ArchonConfig_V1::Private_Data::push(void* to)
{
  char    cm[Pds::Archon::ConfigV1::FILENAME_CHAR_MAX];
  strncpy(cm, _conf->value, Pds::Archon::ConfigV1::FILENAME_CHAR_MAX);
  
  new (to) Pds::Archon::ConfigV1(
    _enumReadoutMode.value,
    _sweepCount.value,
    _integrationTime.value,
    _nonIntegrationTime.value,
    _preSkipPixels.value,
    _pixels.value,
    _postSkipPixels.value,
    _overscanPixels.value,
    _preSkipLines.value,
    _lines.value,
    _postSkipLines.value,
    _overScanLines.value,
    _horizontalBinning.value,
    _verticalBinning.value,
    _rgh.value,
    _rgl.value,
    _shp.value,
    _shd.value,
    _st.value,
    _stm1.value,
    _at.value,
    _dwell1.value,
    _dwell2.value,
    _rgHigh.value,
    _rgLow.value,
    _sHigh.value,
    _sLow.value,
    _aHigh.value,
    _aLow.value,
    _rgSlew.value,
    _sSlew.value,
    _aSlew.value,
    cm
  );
  
  return sizeof(Pds::Archon::ConfigV1);
}
 
Pds_ConfigDb::ArchonConfig_V1::ArchonConfig_V1() :
  Serializer("ArchonConfig_V1"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int Pds_ConfigDb::ArchonConfig_V1::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::ArchonConfig_V1::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::ArchonConfig_V1::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"
