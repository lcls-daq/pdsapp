#define __STDC_LIMIT_MACROS

#include "ZylaConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/ZylaConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


#define CAMERA_MAX_WIDTH  2560
#define CAMERA_MAX_HEIGHT 2160


using namespace Pds_ConfigDb;

class Pds_ConfigDb::ZylaConfig::Private_Data : public Parameter {
  static const char*  lsEnumATBool[];
  static const char*  lsEnumShutteringMode[];
  static const char*  lsEnumFanSpeed[];
  static const char*  lsEnumReadoutRate[];
  static const char*  lsEnumGainMode[];
  static const char*  lsEnumCoolingSetpoint[];
 public:
   Private_Data(bool expert_mode);
  ~Private_Data();

   QLayout* initialize( QWidget* p );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(ZylaConfigType); }
   void flush ()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
   void update()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
    void enable(bool l)
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }

  Pds::LinkedList<Parameter> pList;
  bool _expert_mode;
  Enumerated<ZylaConfigType::ATBool> _cooling;
  Enumerated<ZylaConfigType::ATBool> _overlap;
  Enumerated<ZylaConfigType::ATBool> _noiseFilter;
  Enumerated<ZylaConfigType::ATBool> _blemishCorrection;
  Enumerated<ZylaConfigType::ShutteringMode> _shutter;
  Enumerated<ZylaConfigType::FanSpeed> _fanSpeed;
  Enumerated<ZylaConfigType::ReadoutRate> _readoutRate;
  ZylaConfigType::TriggerMode _triggerMode;
  Enumerated<ZylaConfigType::GainMode> _gainMode;
  Enumerated<ZylaConfigType::CoolingSetpoint> _setpoint;
  NumericInt<uint32_t> _width;
  NumericInt<uint32_t> _height;
  CheckValue           _boxROI;
  NumericInt<uint32_t> _orgX;
  NumericInt<uint32_t> _orgY;
  NumericInt<uint32_t> _binX;
  NumericInt<uint32_t> _binY;
  CheckValue           _boxTrigger;
  NumericFloat<double> _exposureTime;
  NumericFloat<double> _triggerDelay;
  QtConcealer          _concealerTrigger;
  QtConcealer          _concealerROI;
  QtConcealer          _concealerExpert;
};

const char* Pds_ConfigDb::ZylaConfig::Private_Data::lsEnumATBool[] = { "Disabled", "Enabled", NULL };
const char* Pds_ConfigDb::ZylaConfig::Private_Data::lsEnumShutteringMode[] = { "Rolling", "Global", NULL };
const char* Pds_ConfigDb::ZylaConfig::Private_Data::lsEnumFanSpeed[] = { "Off", "Low (Neo Only)", "On", NULL };
const char* Pds_ConfigDb::ZylaConfig::Private_Data::lsEnumReadoutRate[] = { "280 MHz", "200 MHz", "100 MHz", "10 MHz", NULL };
const char* Pds_ConfigDb::ZylaConfig::Private_Data::lsEnumGainMode[] = { "HighWellCap12Bit", "LowNoise12Bit", "LowNoiseHighWellCap16Bit", NULL };
const char* Pds_ConfigDb::ZylaConfig::Private_Data::lsEnumCoolingSetpoint[] = { "0 C", "-5 C", "-10 C", "-15 C", "-20 C", "-25 C", "-30 C", "-35 C", "-40 C", NULL };

Pds_ConfigDb::ZylaConfig::Private_Data::Private_Data(bool expert_mode) :
  _expert_mode              (expert_mode),
  _cooling                  ("Sensor Cooling",              ZylaConfigType::True,             lsEnumATBool),
  _overlap                  ("Overlap Acquistion Mode",     ZylaConfigType::False,            lsEnumATBool),
  _noiseFilter              ("Noise Filter",                ZylaConfigType::False,            lsEnumATBool),
  _blemishCorrection        ("Blemish Correction",          ZylaConfigType::False,            lsEnumATBool),
  _shutter                  ("Shuttering Mode",             ZylaConfigType::Global,           lsEnumShutteringMode),
  _fanSpeed                 ("Fan Speed",                   ZylaConfigType::On,               lsEnumFanSpeed),
  _readoutRate              ("Readout Rate",                ZylaConfigType::Rate280MHz,       lsEnumReadoutRate),
  _triggerMode              (ZylaConfigType::External),
  _gainMode                 ("Gain Mode",                   ZylaConfigType::HighWellCap12Bit, lsEnumGainMode),
  _setpoint                 ("Cooling Setpoint (Neo Only)", ZylaConfigType::Temp_0C,          lsEnumCoolingSetpoint),
  _width                    ("Width",                       2560,   1,        CAMERA_MAX_WIDTH),
  _height                   ("Height",                      2160,   1,        CAMERA_MAX_HEIGHT),
  _boxROI                   ("Centered ROI",                true),
  _orgX                     ("Origin X",                    1,      1,        CAMERA_MAX_WIDTH),
  _orgY                     ("Origin Y",                    1,      1,        CAMERA_MAX_HEIGHT),
  _binX                     ("Binning X",                   1,      1,        CAMERA_MAX_WIDTH),
  _binY                     ("Binning Y",                   1,      1,        CAMERA_MAX_HEIGHT),
  _boxTrigger               ("Exposure from Trigger",       false),
  _exposureTime             ("Exposure Time (sec)",         0.001,  1.0E-05,  30.0),
  _triggerDelay             ("External Trigger Delay",      0.0,    0.0,      19841.5)
{
  pList.insert( &_cooling );
  pList.insert( &_overlap );
  pList.insert( &_noiseFilter );
  pList.insert( &_blemishCorrection );
  pList.insert( &_shutter );
  pList.insert( &_fanSpeed );
  pList.insert( &_readoutRate );
  pList.insert( &_gainMode );
  pList.insert( &_setpoint );
  pList.insert( &_width );
  pList.insert( &_height );
  pList.insert( &_boxROI );
  pList.insert( &_orgX );
  pList.insert( &_orgY );
  pList.insert( &_binX );
  pList.insert( &_binY );
  pList.insert( &_boxTrigger );
  pList.insert( &_exposureTime );
  pList.insert( &_triggerDelay );
}

Pds_ConfigDb::ZylaConfig::Private_Data::~Private_Data() 
{}

QLayout* Pds_ConfigDb::ZylaConfig::Private_Data::initialize(QWidget* p)
{
  QVBoxLayout* layout = new QVBoxLayout;
  { QVBoxLayout* m = new QVBoxLayout;
    m->addWidget(new QLabel("ROI / Image settings: "));
    m->addLayout(_width             .initialize(p));
    m->addLayout(_height            .initialize(p));
    m->addLayout(_boxROI            .initialize(p));
    m->addLayout(_concealerROI.add(_orgX.initialize(p)));
    m->addLayout(_concealerROI.add(_orgY.initialize(p)));
    m->addLayout(_binX              .initialize(p));
    m->addLayout(_binY              .initialize(p));
    m->addLayout(_noiseFilter       .initialize(p));
    m->addLayout(_blemishCorrection .initialize(p));
    m->setSpacing(5);
    layout->addLayout(m); }
  layout->addStretch();
  { QVBoxLayout* t = new QVBoxLayout;
    t->addWidget(new QLabel("Cooling settings: "));
    t->addLayout(_cooling   .initialize(p));
    t->addLayout(_setpoint  .initialize(p));
    t->addLayout(_fanSpeed  .initialize(p));
    t->setSpacing(5);
    layout->addLayout(t); }
  layout->addStretch();
  { QVBoxLayout* d = new QVBoxLayout;
    d->addWidget(new QLabel("Trigger / Readout settings: "));
    d->addLayout(_concealerExpert.add(_shutter.initialize(p)));
    d->addLayout(_gainMode      .initialize(p));
    d->addLayout(_readoutRate   .initialize(p));
    d->addLayout(_overlap.initialize(p));
    d->addLayout(_concealerExpert.add(_boxTrigger.initialize(p)));
    d->addLayout(_concealerTrigger.add(_exposureTime.initialize(p)));
    d->addLayout(_triggerDelay  .initialize(p));
    d->setSpacing(5);
    layout->addLayout(d); }
  layout->setSpacing(25);

  if (Parameter::allowEdit()) {
    QObject::connect(_boxROI._input, SIGNAL(toggled(bool)), &_concealerROI, SLOT(hide(bool)));
    QObject::connect(_boxTrigger._input, SIGNAL(toggled(bool)), &_concealerTrigger, SLOT(hide(bool)));
  }

  return layout;
}

int Pds_ConfigDb::ZylaConfig::Private_Data::pull( void* from )
{
  ZylaConfigType& cfg = * new (from) ZylaConfigType;
  _cooling.value            = cfg.cooling();
  _overlap.value            = cfg.overlap();
  _noiseFilter.value        = cfg.noiseFilter();
  _blemishCorrection.value  = cfg.blemishCorrection();
  _shutter.value            = cfg.shutter();
  _fanSpeed.value           = cfg.fanSpeed();
  _readoutRate.value        = cfg.readoutRate();
  _triggerMode              = cfg.triggerMode();
  _gainMode.value           = cfg.gainMode();
  _setpoint.value           = cfg.setpoint();
  _width.value              = cfg.width();
  _height.value             = cfg.height();
  _orgX.value               = cfg.orgX();
  _orgY.value               = cfg.orgY();
  _boxROI.value             = (((CAMERA_MAX_WIDTH - _width.value) / 2 + 1) == _orgX.value) && (((CAMERA_MAX_HEIGHT - _height.value) / 2 + 1) == _orgY.value);
  _binX.value               = cfg.binX();
  _binY.value               = cfg.binY();
  _exposureTime.value       = cfg.exposureTime();
  _boxTrigger.value         = (_triggerMode == ZylaConfigType::ExternalExposure);
  _triggerDelay.value       = cfg.triggerDelay();

  _concealerROI.show(!_boxROI.value);
  _concealerTrigger.show(!_boxTrigger.value);
  _concealerExpert.show(_expert_mode);

  return sizeof(ZylaConfigType);
}
  
int Pds_ConfigDb::ZylaConfig::Private_Data::push(void* to)
{
  if (_boxROI.value) {
    // using a centered ROI
    _orgX.value = (CAMERA_MAX_WIDTH  - _width.value ) / 2 + 1;
    _orgY.value = (CAMERA_MAX_HEIGHT - _height.value) / 2 + 1;
  }
  if (_boxTrigger.value) {
    // external exposure trigger mode
    _exposureTime.value = 0.0;
    _triggerMode = ZylaConfigType::ExternalExposure;
  } else {
    _triggerMode = ZylaConfigType::External;
  }

  new (to) ZylaConfigType(
    _cooling.value,
    _overlap.value,
    _noiseFilter.value,
    _blemishCorrection.value,
    _shutter.value,
    _fanSpeed.value,
    _readoutRate.value,
    _triggerMode,
    _gainMode.value,
    _setpoint.value,
    _width.value,
    _height.value,
    _orgX.value,
    _orgY.value,
    _binX.value,
    _binY.value,
    _exposureTime.value,
    _triggerDelay.value
  );
  
  return sizeof(ZylaConfigType);
}
 
Pds_ConfigDb::ZylaConfig::ZylaConfig(bool expert_mode) :
  Serializer("ZylaConfig"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::ZylaConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::ZylaConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::ZylaConfig::dataSize() const
{
  return _private_data->dataSize();
}

#undef CAMERA_MAX_WIDTH
#undef CAMERA_MAX_HEIGHT

#include "Parameters.icc"
