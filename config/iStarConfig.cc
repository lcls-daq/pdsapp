#define __STDC_LIMIT_MACROS

#include "iStarConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/ZylaConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


#define CAMERA_MAX_WIDTH  2560
#define CAMERA_MAX_HEIGHT 2160


using namespace Pds_ConfigDb;

class Pds_ConfigDb::iStarConfig::Private_Data : public Parameter {
  static const char*  lsEnumATBool[];
  static const char*  lsEnumFanSpeed[];
  static const char*  lsEnumReadoutRate[];
  static const char*  lsEnumGainMode[];
  static const char*  lsEnumGateMode[];
  static const char*  lsEnumInsertionDelay[];
  static const double ReadoutTimeRow[];
  static const int    ReadoutExtraRows;
  static const int    ReadoutExtraRowsOverlap;
 public:
   Private_Data(bool expert_mode);
  ~Private_Data();

   QLayout* initialize( QWidget* p );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(iStarConfigType); }
   void flush ()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
   void update()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
   void enable(bool l)
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }
   bool   validate();
   double readoutTime(bool overlap) const;

  Pds::LinkedList<Parameter> pList;
  bool _expert_mode;
  bool _short_exposure_mode;
  Enumerated<iStarConfigType::ATBool> _cooling;
  Enumerated<iStarConfigType::ATBool> _overlap;
  Enumerated<iStarConfigType::ATBool> _noiseFilter;
  Enumerated<iStarConfigType::ATBool> _blemishCorrection;
  Enumerated<iStarConfigType::ATBool> _mcpIntelligate;
  Enumerated<iStarConfigType::FanSpeed> _fanSpeed;
  Enumerated<iStarConfigType::ReadoutRate> _readoutRate;
  iStarConfigType::TriggerMode _triggerMode;
  Enumerated<iStarConfigType::GainMode> _gainMode;
  Enumerated<iStarConfigType::GateMode> _gateMode;
  Enumerated<iStarConfigType::InsertionDelay> _insertionDelay;
  NumericInt<uint16_t> _mcpGain;
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

const char* Pds_ConfigDb::iStarConfig::Private_Data::lsEnumATBool[] = { "Disabled", "Enabled", NULL };
const char* Pds_ConfigDb::iStarConfig::Private_Data::lsEnumFanSpeed[] = { "Off", "On", NULL };
const char* Pds_ConfigDb::iStarConfig::Private_Data::lsEnumReadoutRate[] = { "280 MHz", "100 MHz", NULL };
const char* Pds_ConfigDb::iStarConfig::Private_Data::lsEnumGainMode[] = { "HighWellCap12Bit", "LowNoise12Bit", "LowNoiseHighWellCap16Bit", NULL };
const char* Pds_ConfigDb::iStarConfig::Private_Data::lsEnumGateMode[] = { "CWOn", "CWOff", "FireOnly", "GateOnly", "FireAndGate", "DDG", NULL };
const char* Pds_ConfigDb::iStarConfig::Private_Data::lsEnumInsertionDelay[] = { "Normal", "Fast", NULL };
const double Pds_ConfigDb::iStarConfig::Private_Data::ReadoutTimeRow[] = { 9.24E-6, 0.0, 25.41e-6, 0.0 };
const int    Pds_ConfigDb::iStarConfig::Private_Data::ReadoutExtraRows = 4;
const int    Pds_ConfigDb::iStarConfig::Private_Data::ReadoutExtraRowsOverlap = 10;

Pds_ConfigDb::iStarConfig::Private_Data::Private_Data(bool expert_mode) :
  _expert_mode              (expert_mode),
  _short_exposure_mode      (false),
  _cooling                  ("Sensor Cooling",              iStarConfigType::True,             lsEnumATBool),
  _overlap                  ("Overlap Acquistion Mode",     iStarConfigType::False,            lsEnumATBool),
  _noiseFilter              ("Noise Filter",                iStarConfigType::False,            lsEnumATBool),
  _blemishCorrection        ("Blemish Correction",          iStarConfigType::False,            lsEnumATBool),
  _mcpIntelligate           ("MCP Intelligate",             iStarConfigType::False,            lsEnumATBool),
  _fanSpeed                 ("Fan Speed",                   iStarConfigType::On,               lsEnumFanSpeed),
  _readoutRate              ("Readout Rate",                iStarConfigType::Rate280MHz,       lsEnumReadoutRate),
  _triggerMode              (iStarConfigType::External),
  _gainMode                 ("Gain Mode",                   iStarConfigType::HighWellCap12Bit, lsEnumGainMode),
  _gateMode                 ("Gate Mode",                   iStarConfigType::CWOff,            lsEnumGateMode),
  _insertionDelay           ("Insertion Delay",             iStarConfigType::Normal,           lsEnumInsertionDelay),
  _mcpGain                  ("MCP Gain",                    0,      0,        4095),
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
  pList.insert( &_mcpIntelligate );
  pList.insert( &_fanSpeed );
  pList.insert( &_readoutRate );
  pList.insert( &_gainMode );
  pList.insert( &_gateMode );
  pList.insert( &_insertionDelay );
  pList.insert( &_mcpGain );
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

Pds_ConfigDb::iStarConfig::Private_Data::~Private_Data() 
{}

QLayout* Pds_ConfigDb::iStarConfig::Private_Data::initialize(QWidget* p)
{
  QVBoxLayout* layout = new QVBoxLayout;
  { QVBoxLayout* m = new QVBoxLayout;
    m->addWidget(new QLabel("ROI / Image settings: "));
    m->addLayout(_width             .initialize(p));
    m->addLayout(_height            .initialize(p));
    m->addLayout(_boxROI            .initialize(p));
    m->addLayout(_concealerROI.add(_orgX.initialize(p)));
    m->addLayout(_concealerROI.add(_orgY.initialize(p)));
    m->addLayout(_concealerExpert.add(_binX.initialize(p)));
    m->addLayout(_concealerExpert.add(_binY.initialize(p)));
    m->addLayout(_noiseFilter       .initialize(p));
    m->addLayout(_blemishCorrection .initialize(p));
    m->setSpacing(5);
    layout->addLayout(m); }
  layout->addStretch();
  { QVBoxLayout* t = new QVBoxLayout;
    t->addWidget(new QLabel("Cooling settings: "));
    t->addLayout(_cooling   .initialize(p));
    t->addLayout(_fanSpeed  .initialize(p));
    t->setSpacing(5);
    layout->addLayout(t); }
  layout->addStretch();
  { QVBoxLayout* i = new QVBoxLayout;
    i->addWidget(new QLabel("Intensifier settings: "));
    i->addLayout(_gateMode      .initialize(p));
    i->addLayout(_insertionDelay.initialize(p));
    i->addLayout(_mcpIntelligate.initialize(p));
    i->addLayout(_mcpGain       .initialize(p));
    i->setSpacing(5);
    layout->addLayout(i); }
  { QVBoxLayout* d = new QVBoxLayout;
    d->addWidget(new QLabel("Trigger / Readout settings: "));
    d->addLayout(_gainMode      .initialize(p));
    d->addLayout(_readoutRate   .initialize(p));
    d->addLayout(_concealerExpert.add(_overlap.initialize(p)));
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

int Pds_ConfigDb::iStarConfig::Private_Data::pull( void* from )
{
  iStarConfigType& cfg = * new (from) iStarConfigType;
  _cooling.value            = cfg.cooling();
  _overlap.value            = cfg.overlap();
  _noiseFilter.value        = cfg.noiseFilter();
  _blemishCorrection.value  = cfg.blemishCorrection();
  _mcpIntelligate.value     = cfg.mcpIntelligate();
  _fanSpeed.value           = cfg.fanSpeed();
  _readoutRate.value        = cfg.readoutRate();
  _triggerMode              = cfg.triggerMode();
  _gainMode.value           = cfg.gainMode();
  _gateMode.value           = cfg.gateMode();
  _insertionDelay.value     = cfg.insertionDelay();
  _mcpGain.value            = cfg.mcpGain();
  _width.value              = cfg.width();
  _height.value             = cfg.height();
  _orgX.value               = cfg.orgX();
  _orgY.value               = cfg.orgY();
  _boxROI.value             = (((CAMERA_MAX_WIDTH - _width.value) / 2 + 1) == _orgX.value) && (((CAMERA_MAX_HEIGHT - _height.value) / 2 + 1) == _orgY.value);
  _binX.value               = cfg.binX();
  _binY.value               = cfg.binY();
  _exposureTime.value       = cfg.exposureTime();
  _boxTrigger.value         = (_triggerMode == iStarConfigType::ExternalExposure);
  _triggerDelay.value       = cfg.triggerDelay();

  _concealerROI.show(!_boxROI.value);
  _concealerTrigger.show(!_boxTrigger.value);
  _concealerExpert.show(_expert_mode);
  _short_exposure_mode = _exposureTime.value < readoutTime(_overlap.value);

  return sizeof(iStarConfigType);
}
  
int Pds_ConfigDb::iStarConfig::Private_Data::push(void* to)
{
  new (to) iStarConfigType(
    _cooling.value,
    _overlap.value,
    _noiseFilter.value,
    _blemishCorrection.value,
    _mcpIntelligate.value,
    _fanSpeed.value,
    _readoutRate.value,
    _triggerMode,
    _gainMode.value,
    _gateMode.value,
    _insertionDelay.value,
    _mcpGain.value,
    _width.value,
    _height.value,
    _orgX.value,
    _orgY.value,
    _binX.value,
    _binY.value,
    _exposureTime.value,
    _triggerDelay.value
  );
  
  return sizeof(iStarConfigType);
}

double Pds_ConfigDb::iStarConfig::Private_Data::readoutTime(bool overlap) const
{
  int idx = (int) _readoutRate.value;
  unsigned rb = (CAMERA_MAX_HEIGHT / 2) - (_orgY.value - 1);
  unsigned rt = (_orgY.value - 1 + _height.value) - (CAMERA_MAX_HEIGHT / 2);
  return ReadoutTimeRow[idx]*((rb > rt ? rb : rt) + (overlap ? ReadoutExtraRowsOverlap : ReadoutExtraRows));
}

bool Pds_ConfigDb::iStarConfig::Private_Data::validate()
{
  // make sure all the values are updated
  if (_boxROI.value) {
    // using a centered ROI
    _orgX.value = (CAMERA_MAX_WIDTH  - _width.value ) / 2 + 1;
    _orgY.value = (CAMERA_MAX_HEIGHT - _height.value) / 2 + 1;
  }
  if (_boxTrigger.value) {
    // external exposure trigger mode
    _exposureTime.value = 0.0;
    _triggerMode = iStarConfigType::ExternalExposure;
  } else {
    _triggerMode = iStarConfigType::External;
  }

  if ((_readoutRate.value != iStarConfigType::Rate280MHz) && (_readoutRate.value != iStarConfigType::Rate100MHz)) {
    QString msg = QString("The camera does not support the %1 readout speed setting!")
      .arg(lsEnumReadoutRate[_readoutRate.value]);
    QMessageBox::critical(0, "Invalid Readout Speed", msg);
    return false;
  }

  double readout_time = readoutTime(_overlap.value);

  if (_triggerMode == iStarConfigType::ExternalExposure) {
    return true;
  } else if (_overlap.value) {
    if (_exposureTime.value < readout_time) {
      QString msg = QString("The selected exposure time of %1 sec is shorter than the camera readout time of %2 sec!\n\nOverlap mode does not support short exposures.\n")
        .arg(_exposureTime.value)
        .arg(readout_time);
        QMessageBox::critical(0, "Invalid Exposure Time", msg);
        return false;
    } else {
      return true;
    }
  } else {
    if (_exposureTime.value < readout_time ? _short_exposure_mode : !_short_exposure_mode) return true;
    QString msg;
    const char* base_msg = "The selected exposure time of %1 sec is %3er than the camera readout time of %2 sec!\n\n%4\n\nMove camera to %3 exposure mode?";
    if (_exposureTime.value < readout_time) {
      msg = QString(base_msg)
        .arg(_exposureTime.value)
        .arg(readout_time)
        .arg("short")
        .arg("This requires triggering on an earlier fiducial than the beam!");
    } else {
      msg = QString(base_msg)
        .arg(_exposureTime.value)
        .arg(readout_time)
        .arg("long")
        .arg("This requires triggering on the same fiducial as the beam!");
    }
    switch (QMessageBox::warning(0,"Confirm Exposure Mode Change", msg, "Confirm", "Cancel", 0, 0, 1))
      {
      case 0:
        return true;
      case 1:
        return false;
      }
  }

  return false;
}
 
Pds_ConfigDb::iStarConfig::iStarConfig(bool expert_mode) :
  Serializer("iStarConfig"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::iStarConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::iStarConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::iStarConfig::dataSize() const
{
  return _private_data->dataSize();
}

bool Pds_ConfigDb::iStarConfig::validate()
{
  return _private_data->validate();
}

#undef CAMERA_MAX_WIDTH
#undef CAMERA_MAX_HEIGHT

#include "Parameters.icc"
