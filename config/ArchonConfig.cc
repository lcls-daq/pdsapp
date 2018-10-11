#include "pdsapp/config/ArchonConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/ArchonConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


using namespace Pds_ConfigDb;

class Pds_ConfigDb::ArchonConfig::Private_Data : public Parameter {
public:
  Private_Data(bool expert_mode);
  ~Private_Data();

  QLayout* initialize( QWidget* p );
  int pull( void* from );
  int push( void* to );
  int dataSize() const
     { ArchonConfigType cfg(_conf.length()); return cfg._sizeof(); }
  void flush ()
     { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
  void update()
     { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
  void enable(bool l)
     { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }
  bool validate();

  Pds::LinkedList<Parameter> pList;
  Enumerated<ArchonConfigType::ReadoutMode>  _enumReadoutMode;
  NumericInt<uint16_t>  _exposureEventCode;
  NumericInt<uint32_t>  _preFrameSweepCount;
  NumericInt<uint32_t>  _idleSweepCount;
  NumericInt<uint32_t>  _integrationTime;
  NumericInt<uint32_t>  _nonIntegrationTime;
  //-------- Image Formatting parameters --------
  NumericInt<uint32_t>  _batches;
  NumericInt<uint32_t>  _pixels;
  NumericInt<uint32_t>  _lines;
  NumericInt<uint32_t>  _horizontalBinning;
  NumericInt<uint32_t>  _verticalBinning;
  //-------- Sensor Info Parameters --------
  NumericInt<uint32_t>  _sensorPixels;
  NumericInt<uint32_t>  _sensorLines;
  NumericInt<uint32_t>  _sensorTaps;
  //-------- Timing Parameters --------
  NumericInt<uint32_t>  _st;
  NumericInt<uint32_t>  _stm1;
  NumericInt<uint32_t>  _at;

  TextFileParameter     _conf;
  QtConcealer           _concealerExpert;
private:
  bool _expert_mode;
  static const char*  lsEnumReadoutMode[];
};

const char* Pds_ConfigDb::ArchonConfig::Private_Data::lsEnumReadoutMode[] = {
  "Single",
  "Continuous",
  "Triggered",
  NULL
};

Pds_ConfigDb::ArchonConfig::Private_Data::Private_Data(bool expert_mode) :
  _enumReadoutMode    ("Readout Mode",          ArchonConfigType::Triggered, lsEnumReadoutMode),
  _exposureEventCode  ("Exposure Event Code",       1,    1,      255),
  _preFrameSweepCount ("Pre-Frame Clear Count",     0,    0,      0xFFFFF),
  _idleSweepCount     ("Idle Clear Count",          1,    0,      0xFFFFF),
  _integrationTime    ("Integration Time (ms)",     10,   0,      0xFFFFF),
  _nonIntegrationTime ("Non-Integration Time (ms)", 100,  0,      0xFFFFF),
  _batches            ("Readout Batch Size",        0,    0,      0xFFFFF),
  _pixels             ("Pixel Count",               300,  0,      0xFFFFF),
  _lines              ("Line Count",                300,  0,      0xFFFFF),
  _horizontalBinning  ("Horizontal Binning",        0,    0,      0xFFFFF),
  _verticalBinning    ("Vertical Binning",          1,    0,      0xFFFFF),
  _sensorPixels       ("Sensor Pixels",             264,  0,      0xFFFFF),
  _sensorLines        ("Sensor Lines",              256,  0,      0xFFFFF),
  _sensorTaps         ("Sensor Taps",               16,   0,      0xFFFFF),
  _st                 ("ST",                        30,   0,      0xFFFFF),
  _stm1               ("STM1",                      29,   0,      0xFFFFF),
  _at                 ("AT",                        2000, 0,      0xFFFFF),
  _conf               ("Archon Configuration File", ArchonConfigMaxFileLength, "Archon Configuration Files (*.acf)"),
  _expert_mode        (expert_mode)
{
  pList.insert( &_enumReadoutMode );
  pList.insert( &_exposureEventCode );
  pList.insert( &_preFrameSweepCount );
  pList.insert( &_idleSweepCount );
  pList.insert( &_integrationTime );
  pList.insert( &_nonIntegrationTime );
  pList.insert( &_batches );
  pList.insert( &_pixels );
  pList.insert( &_lines );
  pList.insert( &_horizontalBinning );
  pList.insert( &_verticalBinning );
  pList.insert( &_sensorPixels );
  pList.insert( &_sensorLines );
  pList.insert( &_sensorTaps );
  pList.insert( &_st );
  pList.insert( &_stm1 );
  pList.insert( &_at );
  pList.insert( &_conf);
}

Pds_ConfigDb::ArchonConfig::Private_Data::~Private_Data()
{
}

QLayout* Pds_ConfigDb::ArchonConfig::Private_Data::initialize(QWidget* p)
{
  QVBoxLayout* layout = new QVBoxLayout;
  { QVBoxLayout* e = new QVBoxLayout;
    e->addWidget(new QLabel("Exposure settings: "));
    e->addLayout(_integrationTime   .initialize(p));
    e->addLayout(_nonIntegrationTime.initialize(p));
    e->addLayout(_preFrameSweepCount.initialize(p));
    e->addLayout(_idleSweepCount    .initialize(p));
    e->setSpacing(5);
    layout->addLayout(e); }
  { QVBoxLayout* i = new QVBoxLayout;
    i->addWidget(new QLabel("Image settings: "));
    i->addLayout(_pixels            .initialize(p));
    i->addLayout(_lines             .initialize(p));
    i->addLayout(_horizontalBinning .initialize(p));
    i->addLayout(_verticalBinning   .initialize(p));
    i->addLayout(_concealerExpert.add(_batches.initialize(p)));
    i->setSpacing(5);
    layout->addLayout(i); }
  { QVBoxLayout* s = new QVBoxLayout;
    s->addWidget(new QLabel("Sensor settings: "));
    s->addLayout(_sensorPixels.initialize(p));
    s->addLayout(_sensorLines .initialize(p));
    s->addLayout(_sensorTaps  .initialize(p));
    s->setSpacing(5);
    layout->addLayout(s); }
  { QVBoxLayout* t = new QVBoxLayout;
    t->addWidget(new QLabel("Timing settings: "));
    t->addLayout(_st  .initialize(p));
    t->addLayout(_stm1.initialize(p));
    t->addLayout(_at  .initialize(p));
    t->setSpacing(5);
    layout->addLayout(t); }
  { QVBoxLayout* x = new QVBoxLayout;
    x->addWidget(new QLabel("Expert settings: "));
    x->addLayout(_enumReadoutMode   .initialize(p));
    x->addLayout(_exposureEventCode .initialize(p));
    x->addLayout(_conf              .initialize(p));
    x->setSpacing(5);
    layout->addLayout(_concealerExpert.add(x)); }
  layout->setSpacing(25);

  return layout;
}

int Pds_ConfigDb::ArchonConfig::Private_Data::pull( void* from )
{
  ArchonConfigType& cfg = * new (from) ArchonConfigType;
  _enumReadoutMode.value    = (ArchonConfigType::ReadoutMode) cfg.readoutMode();
  _exposureEventCode.value  = cfg.exposureEventCode();
  _preFrameSweepCount.value = cfg.preFrameSweepCount();
  _idleSweepCount.value     = cfg.idleSweepCount();
  _integrationTime.value    = cfg.integrationTime();
  _nonIntegrationTime.value = cfg.nonIntegrationTime();
  _batches.value            = cfg.batches();
  _pixels.value             = cfg.pixels();
  _lines.value              = cfg.lines();
  _horizontalBinning.value  = cfg.horizontalBinning();
  _verticalBinning.value    = cfg.verticalBinning();
  _sensorPixels.value       = cfg.sensorPixels();
  _sensorLines.value        = cfg.sensorLines();
  _sensorTaps.value         = cfg.sensorTaps();
  _st.value                 = cfg.st();
  _stm1.value               = cfg.stm1();
  _at.value                 = cfg.at();
  _conf.set_value(cfg.config());

  _concealerExpert.show(_expert_mode);

  return cfg._sizeof();
}
  
int Pds_ConfigDb::ArchonConfig::Private_Data::push(void* to)
{
  ArchonConfigType& cfg = *new (to) ArchonConfigType(
    _enumReadoutMode.value,
    _exposureEventCode.value,
    _conf.length(),
    _preFrameSweepCount.value,
    _idleSweepCount.value,
    _integrationTime.value,
    _nonIntegrationTime.value,
    _batches.value,
    _pixels.value,
    _lines.value,
    _horizontalBinning.value,
    _verticalBinning.value,
    _sensorPixels.value,
    _sensorLines.value,
    _sensorTaps.value,
    _st.value,
    _stm1.value,
    _at.value,
    _conf.value
  );

  return cfg._sizeof();
}

bool Pds_ConfigDb::ArchonConfig::Private_Data::validate()
{
  return true;
}
 
Pds_ConfigDb::ArchonConfig::ArchonConfig(bool expert_mode) :
  Serializer("ArchonConfig"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::ArchonConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::ArchonConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::ArchonConfig::dataSize() const
{
  return _private_data->dataSize();
}

bool Pds_ConfigDb::ArchonConfig::validate()
{
  return _private_data->validate();
}

#include "Parameters.icc"
