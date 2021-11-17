#define __STDC_LIMIT_MACROS

#include "AlviumConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pdsapp/config/QtChoiceConcealer.hh"
#include "pds/config/VimbaConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>

using namespace Pds_ConfigDb;

class Pds_ConfigDb::AlviumConfig::Private_Data : public Parameter {
  static const uint32_t CAMERA_MAX_WIDTH = 5328;
  static const uint32_t CAMERA_WIDTH_STEP = 8;
  static const uint32_t CAMERA_MAX_HEIGHT = 4608;
  static const uint32_t CAMERA_HEIGHT_STEP = 2;
  static const uint32_t CAMERA_MIN_ROI = 8;
  static const uint32_t CAMERA_OFFSET_STEP = 2;
  // note DESC_CHAR_MAX is the max storage size including null termination of the string
  static const int MAX_DESC_STR_LENGTH = AlviumConfigType::DESC_CHAR_MAX - 1;
  static const char*  enumVmbBool[];
  static const char*  enumRoiMode[];
  static const char*  enumTriggerMode[];
  static const char*  enumPixelMode[];
  static const char*  enumImgCorrectionType[];
  static const char*  enumImgCorrectionSet[];
  public:
    Private_Data(bool expert_mode);
    ~Private_Data();

    QLayout* initialize( QWidget* p );
    int pull( void* from );
    int push( void* to );
    int dataSize() const
      { return sizeof(AlviumConfigType); }
   void flush ()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
   void update()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
   void enable(bool l)
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }
    bool validate();

    Pds::LinkedList<Parameter> pList;
    bool _expert_mode;
    Enumerated<AlviumConfigType::VmbBool> _reverseX;
    Enumerated<AlviumConfigType::VmbBool> _reverseY;
    Enumerated<AlviumConfigType::VmbBool> _contrastEnable;
    Enumerated<AlviumConfigType::VmbBool> _correctionEnable;
    Enumerated<AlviumConfigType::RoiMode> _roiEnable;
    Enumerated<AlviumConfigType::ImgCorrectionType> _correctionType;
    Enumerated<AlviumConfigType::ImgCorrectionSet> _correctionSet;
    Enumerated<AlviumConfigType::PixelMode> _pixelMode;
    Enumerated<AlviumConfigType::TriggerMode> _triggerMode;
    NumericInt<uint32_t> _width;
    NumericInt<uint32_t> _height;
    NumericInt<uint32_t> _offsetX;
    NumericInt<uint32_t> _offsetY;
    NumericInt<uint32_t> _sensorWidth;
    NumericInt<uint32_t> _sensorHeight;
    NumericInt<uint32_t> _contrastDarkLimit;
    NumericInt<uint32_t> _contrastBrightLimit;
    NumericInt<uint32_t> _contrastShape;
    NumericFloat<double> _exposureTime;
    NumericFloat<double> _blackLevel;
    NumericFloat<double> _gain;
    NumericFloat<double> _gamma;
    TextParameter        _manufacturer;
    TextParameter        _family;
    TextParameter        _model;
    TextParameter        _manufacturerId;
    TextParameter        _version;
    TextParameter        _serialNumber;
    TextParameter        _firmwareId;
    TextParameter        _firmwareVersion;
    QtConcealer          _concealerContrast;
    QtConcealer          _concealerCorrection;
    QtConcealer          _concealerExpert;
    QtConcealer          _concealerReadOnly;
    QtChoiceConcealer    _concealerROI;
};

const char* Pds_ConfigDb::AlviumConfig::Private_Data::enumVmbBool[] = { "False", "True", NULL };
const char* Pds_ConfigDb::AlviumConfig::Private_Data::enumRoiMode[] = { "Off", "On", "Centered", NULL };
const char* Pds_ConfigDb::AlviumConfig::Private_Data::enumTriggerMode[] = { "FreeRun", "External", "Software", NULL };
const char* Pds_ConfigDb::AlviumConfig::Private_Data::enumPixelMode[] = { "8 Bit", "10 Bit", "10 Bit Packed", "12 Bit", "12 Bit Packed", NULL };
const char* Pds_ConfigDb::AlviumConfig::Private_Data::enumImgCorrectionType[] = { "DefectPixelCorrection", "FixedPatternNoiseCorrection", NULL };
const char* Pds_ConfigDb::AlviumConfig::Private_Data::enumImgCorrectionSet[] = { "Preset", "User", NULL };

Pds_ConfigDb::AlviumConfig::Private_Data::Private_Data(bool expert_mode) :
  _expert_mode        (expert_mode),
  _reverseX           ("Flip X",                AlviumConfigType::False,                  enumVmbBool),
  _reverseY           ("Flip Y",                AlviumConfigType::False,                  enumVmbBool),
  _contrastEnable     ("Contrast Enhancement",  AlviumConfigType::False,                  enumVmbBool),
  _correctionEnable   ("Pixel Correction",      AlviumConfigType::False,                  enumVmbBool),
  _roiEnable          ("ROI",                   AlviumConfigType::Off,                    enumRoiMode),
  _correctionType     ("Correction Type",       AlviumConfigType::DefectPixelCorrection,  enumImgCorrectionType),
  _correctionSet      ("Correction Set",        AlviumConfigType::Preset,                 enumImgCorrectionSet),
  _pixelMode          ("Depth",                 AlviumConfigType::Mono8,                  enumPixelMode),
  _triggerMode        ("Trigger",               AlviumConfigType::External,               enumTriggerMode),
  _width              ("Width",                 1936,   1,        CAMERA_MAX_WIDTH),
  _height             ("Height",                1216,   1,        CAMERA_MAX_HEIGHT),
  _offsetX            ("Offset X",              0,      0,        CAMERA_MAX_WIDTH),
  _offsetY            ("Offset Y",              0,      0,        CAMERA_MAX_HEIGHT),
  _sensorWidth        ("Sensor Width",          0,      0,        CAMERA_MAX_WIDTH),
  _sensorHeight       ("Sensor Height",         0,      0,        CAMERA_MAX_HEIGHT),
  _contrastDarkLimit  ("Contrast Dark Limit",   0,      0,        4094),
  _contrastBrightLimit("Contrast Bright Limit", 1,      1,        4095),
  _contrastShape      ("Contrast Shape",        4,      1,        10),
  _exposureTime       ("Exposure Time",         0.002,  0.00018,  10.0),
  _blackLevel         ("Black Level",           0.0,    0.0,      255.0),
  _gain               ("Gain",                  0.0,    0.0,      24.0),
  _gamma              ("Gamma",                 1.0,    0.40,     2.40),
  _manufacturer       ("Manufacturer",          "",     MAX_DESC_STR_LENGTH),
  _family             ("Model Family",          "",     MAX_DESC_STR_LENGTH),
  _model              ("Model Name",            "",     MAX_DESC_STR_LENGTH),
  _manufacturerId     ("Manufacturer ID",       "",     MAX_DESC_STR_LENGTH),
  _version            ("Hardware Version",      "",     MAX_DESC_STR_LENGTH),
  _serialNumber       ("Serial Number",         "",     MAX_DESC_STR_LENGTH),
  _firmwareId         ("Firmware ID",           "",     MAX_DESC_STR_LENGTH),
  _firmwareVersion    ("Firmware Version",      "",     MAX_DESC_STR_LENGTH)
{
  pList.insert( &_reverseX );
  pList.insert( &_reverseY );
  pList.insert( &_contrastEnable );
  pList.insert( &_correctionEnable );
  pList.insert( &_roiEnable );
  pList.insert( &_correctionType );
  pList.insert( &_correctionSet );
  pList.insert( &_pixelMode );
  pList.insert( &_triggerMode );
  pList.insert( &_width );
  pList.insert( &_height );
  pList.insert( &_offsetX );
  pList.insert( &_offsetY );
  pList.insert( &_sensorWidth );
  pList.insert( &_sensorHeight );
  pList.insert( &_contrastDarkLimit );
  pList.insert( &_contrastBrightLimit );
  pList.insert( &_contrastShape );
  pList.insert( &_exposureTime );
  pList.insert( &_blackLevel );
  pList.insert( &_gain );
  pList.insert( &_gamma );
  pList.insert( &_manufacturer );
  pList.insert( &_family );
  pList.insert( &_model );
  pList.insert( &_manufacturerId );
  pList.insert( &_version );
  pList.insert( &_serialNumber );
  pList.insert( &_firmwareId );
  pList.insert( &_firmwareVersion );
}

Pds_ConfigDb::AlviumConfig::Private_Data::~Private_Data()
{}

QLayout* Pds_ConfigDb::AlviumConfig::Private_Data::initialize(QWidget* p)
{
  QVBoxLayout* layout = new QVBoxLayout;
  { QVBoxLayout* d = new QVBoxLayout;
    d->addWidget(new QLabel("Device Info: "));
    d->addLayout(_manufacturer    .initialize(p));
    d->addLayout(_family          .initialize(p));
    d->addLayout(_model           .initialize(p));
    d->addLayout(_manufacturerId  .initialize(p));
    d->addLayout(_version         .initialize(p));
    d->addLayout(_serialNumber    .initialize(p));
    d->addLayout(_firmwareId      .initialize(p));
    d->addLayout(_firmwareVersion.initialize(p));
    d->setSpacing(5);
    layout->addLayout(_concealerReadOnly.add(d)); }
  { QVBoxLayout* m = new QVBoxLayout;
    m->addWidget(new QLabel("ROI / Image settings: "));
    m->addLayout(_roiEnable.initialize(p));
    m->addLayout(_concealerROI.add(_width   .initialize(p), 1<<AlviumConfigType::On | 1<<AlviumConfigType::Centered));
    m->addLayout(_concealerROI.add(_height  .initialize(p), 1<<AlviumConfigType::On | 1<<AlviumConfigType::Centered));
    m->addLayout(_concealerROI.add(_offsetX .initialize(p), 1<<AlviumConfigType::On));
    m->addLayout(_concealerROI.add(_offsetY .initialize(p), 1<<AlviumConfigType::On));
    m->addLayout(_concealerReadOnly.add(_sensorWidth .initialize(p)));
    m->addLayout(_concealerReadOnly.add(_sensorHeight.initialize(p)));
    m->addLayout(_reverseX.initialize(p));
    m->addLayout(_reverseY.initialize(p));
    m->addLayout(_contrastEnable.initialize(p));
    m->addLayout(_concealerContrast.add(_contrastDarkLimit  .initialize(p)));
    m->addLayout(_concealerContrast.add(_contrastBrightLimit.initialize(p)));
    m->addLayout(_concealerContrast.add(_contrastShape      .initialize(p)));
    m->addLayout(_correctionEnable.initialize(p));
    m->addLayout(_concealerCorrection.add(_correctionType.initialize(p)));
    m->addLayout(_concealerCorrection.add(_correctionSet.initialize(p)));
    m->setSpacing(5);
    layout->addLayout(m); }
  { QVBoxLayout* t = new QVBoxLayout;
    t->addWidget(new QLabel("Trigger / Readout settings: "));
    t->addLayout(_pixelMode   .initialize(p));
    t->addLayout(_blackLevel  .initialize(p));
    t->addLayout(_gain        .initialize(p));
    t->addLayout(_gamma       .initialize(p));
    t->addLayout(_concealerExpert.add(_triggerMode.initialize(p)));
    t->addLayout(_exposureTime.initialize(p));
    t->setSpacing(5);
    layout->addLayout(t); }
  layout->setSpacing(25);

  if (Parameter::allowEdit()) {
    QObject::connect(_contrastEnable._input, SIGNAL(currentIndexChanged(int)), &_concealerContrast, SLOT(show(int)));
    QObject::connect(_correctionEnable._input, SIGNAL(currentIndexChanged(int)), &_concealerCorrection, SLOT(show(int)));
    QObject::connect(_roiEnable._input, SIGNAL(currentIndexChanged(int)), &_concealerROI, SLOT(show(int)));
  }

  return layout;
}

int Pds_ConfigDb::AlviumConfig::Private_Data::pull( void* from )
{
  AlviumConfigType& cfg = * new (from) AlviumConfigType;
  _reverseX.value             = cfg.reverseX();
  _reverseY.value             = cfg.reverseY();
  _contrastEnable.value       = cfg.contrastEnable();
  _correctionEnable.value     = cfg.correctionEnable();
  _roiEnable.value            = cfg.roiEnable();
  _correctionType.value       = cfg.correctionType();
  _correctionSet.value        = cfg.correctionSet();
  _pixelMode.value            = cfg.pixelMode();
  _triggerMode.value          = cfg.triggerMode();
  _width.value                = cfg.width();
  _height.value               = cfg.height();
  _offsetX.value              = cfg.offsetX();
  _offsetY.value              = cfg.offsetY();
  _sensorWidth.value          = cfg.sensorWidth();
  _sensorHeight.value         = cfg.sensorHeight();
  _contrastDarkLimit.value    = cfg.contrastDarkLimit();
  _contrastBrightLimit.value  = cfg.contrastBrightLimit();
  _contrastShape.value        = cfg.contrastShape();
  _exposureTime.value         = cfg.exposureTime();
  _blackLevel.value           = cfg.blackLevel();
  _gain.value                 = cfg.gain();
  _gamma.value                = cfg.gamma();

  strcpy(_manufacturer.value,     cfg.manufacturer());
  strcpy(_family.value,           cfg.family());
  strcpy(_model.value,            cfg.model());
  strcpy(_manufacturerId.value,   cfg.manufacturerId());
  strcpy(_version.value,          cfg.version());
  strcpy(_serialNumber.value,     cfg.serialNumber());
  strcpy(_firmwareId.value,       cfg.firmwareId());
  strcpy(_firmwareVersion.value,  cfg.firmwareVersion());

  _concealerContrast.show(_contrastEnable.value);
  _concealerCorrection.show(_correctionEnable.value);
  _concealerROI.show(_roiEnable.value);
  _concealerExpert.show(_expert_mode);
  _concealerReadOnly.hide(Parameter::allowEdit());

  return sizeof(AlviumConfigType);
}

int Pds_ConfigDb::AlviumConfig::Private_Data::push(void* to)
{
  new (to) AlviumConfigType(
    _reverseX.value,
    _reverseY.value,
    _contrastEnable.value,
    _correctionEnable.value,
    _roiEnable.value,
    _correctionType.value,
    _correctionSet.value,
    _pixelMode.value,
    _triggerMode.value,
    _width.value,
    _height.value,
    _offsetX.value,
    _offsetY.value,
    _sensorWidth.value,
    _sensorHeight.value,
    _contrastDarkLimit.value,
    _contrastBrightLimit.value,
    _contrastShape.value,
    _exposureTime.value,
    _blackLevel.value,
    _gain.value,
    _gamma.value,
    _manufacturer.value,
    _family.value,
    _model.value,
    _manufacturerId.value,
    _version.value,
    _serialNumber.value,
    _firmwareId.value,
    _firmwareVersion.value
  );

  return sizeof(AlviumConfigType);
}

bool Pds_ConfigDb::AlviumConfig::Private_Data::validate()
{
  switch (_roiEnable.value) {
    case AlviumConfigType::Off:
      // Nothing to check in this case!
      break;
    case AlviumConfigType::On:
      // check the ROI offset matches the step size
      if ((_offsetX.value % CAMERA_OFFSET_STEP != 0) || (_offsetY.value % CAMERA_OFFSET_STEP != 0)) {
        QString msg = QString("The camera ROI offsets must be a multiple of %1!")
          .arg(CAMERA_OFFSET_STEP);
        QMessageBox::critical(0, "Invalid ROI", msg);
        return false;
      }
    case AlviumConfigType::Centered:
      // make sure the ROI width matches the step size
      if (_width.value % CAMERA_WIDTH_STEP != 0) {
        QString msg = QString("The camera ROI width must be a multiple of %1!")
          .arg(CAMERA_WIDTH_STEP);
        QMessageBox::critical(0, "Invalid ROI", msg);
        return false;
      }

      // make sure the ROI height matches the step size
      if (_height.value % CAMERA_HEIGHT_STEP != 0) {
        QString msg = QString("The camera ROI height must be a multiple of %1!")
          .arg(CAMERA_HEIGHT_STEP);
        QMessageBox::critical(0, "Invalid ROI", msg);
        return false;
      }

      // check the minimum size of the ROI
      if ((_height.value < CAMERA_MIN_ROI) || (_width.value < CAMERA_MIN_ROI)) {
        QString msg = QString("The camera ROI must be a mimimum of an %1 x %1 square!")
          .arg(CAMERA_MIN_ROI);
        QMessageBox::critical(0, "Invalid ROI", msg);
        return false;
      }

      break;
    default:
      QString msg = QString("Invalid ROI mode of %1!")
          .arg(_roiEnable.value);
      QMessageBox::critical(0, "Invalid ROI", msg);
      return false;
  }

  // check using external trigger and warn if not
  if ((_triggerMode.value != AlviumConfigType::External) && (!_expert_mode)) {
    QString msg = QString("The camera is set to a non-external trigger mode '%1', and will not be synchronized to the beam!")
      .arg(enumTriggerMode[_triggerMode.value]);
    switch (QMessageBox::warning(0,"Confirm Trigger Mode Change", msg, "Confirm", "Cancel", 0, 0, 1)) {
    case 0:
      return true;
    case 1:
      return false;
    default:
      return false;
    }
  }

  return true;
}

Pds_ConfigDb::AlviumConfig::AlviumConfig(bool expert_mode) :
  Serializer("AlviumConfig"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::AlviumConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::AlviumConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::AlviumConfig::dataSize() const
{
  return _private_data->dataSize();
}

bool Pds_ConfigDb::AlviumConfig::validate()
{
  return _private_data->validate();
}

bool Pds_ConfigDb::AlviumConfig::fixedSize()
{
  return true;
}

#include "Parameters.icc"
