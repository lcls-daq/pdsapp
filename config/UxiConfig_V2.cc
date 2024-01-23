#include "pdsapp/config/UxiConfig_V2.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/UxiConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>

#include <math.h>
#include <new>

using namespace Pds_ConfigDb;

class Pds_ConfigDb::UxiConfig_V2::Private_Data : public Parameter {
  static const uint32_t MAX_ROWS = 4095;
  static const uint32_t MAX_FRAMES = 7;
public:
  Private_Data(bool expert_mode);
  ~Private_Data();

   QLayout* initialize( QWidget* p );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(Pds::Uxi::ConfigV2); }
   void flush ()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
   void update()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
   void enable(bool l)
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }
   bool   validate();

  Pds::LinkedList<Parameter> pList;
  Enumerated<Pds::Uxi::ConfigV2::RoiMode> _roiParam;
  NumericInt<uint32_t>  _firstRowParam;
  NumericInt<uint32_t>  _lastRowParam;
  NumericInt<uint32_t>  _firstFrameParam;
  NumericInt<uint32_t>  _lastFrameParam;
  NumericInt<uint32_t>  _timeOnParam;
  NumericInt<uint32_t>  _timeOffParam;
  NumericInt<uint32_t>  _delayParam;
  NumericInt<uint32_t>* _timeOnSideParam[Pds::Uxi::ConfigV2::NumberOfSides];
  NumericInt<uint32_t>* _timeOffSideParam[Pds::Uxi::ConfigV2::NumberOfSides];
  NumericInt<uint32_t>* _delaySideParam[Pds::Uxi::ConfigV2::NumberOfSides];
  NumericFloat<double>* _potsParam[Pds::Uxi::ConfigV2::NumberOfPots];
  CheckValue*           _tunePotsParam[Pds::Uxi::ConfigV2::NumberOfPots];
  CheckValue*           _roPotsParam[Pds::Uxi::ConfigV2::NumberOfPots];
  QtConcealer           _concealerExpert;
  QtConcealer           _concealerNonExpert;
  QtConcealer           _concealerROI;
  QtConcealer           _concealerReadOnly[Pds::Uxi::ConfigV2::NumberOfPots];
private:
  bool _expert_mode;
  uint32_t _width;
  uint32_t _height;
  uint32_t _numberOfFrames;
  uint32_t _numberOFBytesPerPixel;
  uint32_t _sensorType;
  uint32_t _timeOn[Pds::Uxi::ConfigV2::NumberOfSides];
  uint32_t _timeOff[Pds::Uxi::ConfigV2::NumberOfSides];
  uint32_t _delay[Pds::Uxi::ConfigV2::NumberOfSides];
  uint32_t _readOnlyPots;
  double   _pots[Pds::Uxi::ConfigV2::NumberOfPots];
  static const char*  roiNames[];
  static const char*  sideNames[];
  static const char*  potNames[];
  static const double initialPotValues[Pds::Uxi::ConfigV2::NumberOfPots];
};

const char*   Pds_ConfigDb::UxiConfig_V2::Private_Data::roiNames[] = {
  "Off",
  "On",
  NULL,
};

const char*   Pds_ConfigDb::UxiConfig_V2::Private_Data::sideNames[] = {
  "Timing settings (Side A): ",
  "Timing settings (Side B): ",
  NULL,
};

const char*   Pds_ConfigDb::UxiConfig_V2::Private_Data::potNames[] = {
  "COL_BOT_IBIAS_IN", // POT1
  "HST_A_PDELAY",     // POT2
  "HST_B_NDELAY",     // POT3
  "HST_RO_IBIAS",     // POT4
  "HST_OSC_VREF_IN",  // POT5
  "HST_B_PDELAY",     // POT6
  "HST_OSC_CTL",      // POT7
  "HST_A_NDELAY",     // POT8
  "COL_TOP_IBIAS_IN", // POT9
  "HST_OSC_R_BIAS",   // POT10
  "VAB",              // POT11
  "HST_RO_NC_IBIAS",  // POT12
  "VRST",             // POT13
};
const double  Pds_ConfigDb::UxiConfig_V2::Private_Data::initialPotValues[] = {
  0.0, 0.0, 0.0, 0.0, 2.8, 0.0, 1.53, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};


Pds_ConfigDb::UxiConfig_V2::Private_Data::Private_Data(bool expert_mode) :
  _roiParam             ("ROI Mode", Pds::Uxi::ConfigV2::Off, roiNames),
  _firstRowParam        ("First Row",           0,  0,  MAX_ROWS),
  _lastRowParam         ("Last Row",            0,  0,  MAX_ROWS),
  _firstFrameParam      ("First Frame",         0,  0,  MAX_FRAMES),
  _lastFrameParam       ("Last Frame",          0,  0,  MAX_FRAMES),
  _timeOnParam          ("Time On",             2,  1,  9999),
  _timeOffParam         ("Time Off",            2,  1,  9999),
  _delayParam           ("Initial delay (ns)",  0,  0,  9999),
  _expert_mode          (expert_mode),
  _width                (512),
  _height               (1024),
  _numberOfFrames       (2),
  _numberOFBytesPerPixel(2),
  _sensorType           (1),
  _readOnlyPots         (8111)
{
  pList.insert( &_roiParam );
  pList.insert( &_firstRowParam );
  pList.insert( &_lastRowParam );
  pList.insert( &_firstFrameParam );
  pList.insert( &_lastFrameParam );
  pList.insert( &_timeOnParam );
  pList.insert( &_timeOffParam );
  pList.insert( &_delayParam );
  for (unsigned iside=0; iside<Pds::Uxi::ConfigV2::NumberOfSides; iside++) {
    _timeOnSideParam[iside] = new NumericInt<uint32_t>("Time On", 2, 1, 9999);
    _timeOffSideParam[iside] = new NumericInt<uint32_t>("Time Off", 2, 1, 9999);
    _delaySideParam[iside] = new NumericInt<uint32_t>("Initial delay (ns)",  0,  0,  9999);
    pList.insert( _timeOnSideParam[iside] );
    pList.insert( _timeOffSideParam[iside] );
    pList.insert( _delaySideParam[iside] );
  }
  for (unsigned ipot=0; ipot<Pds::Uxi::ConfigV2::NumberOfPots; ipot++) {
    _potsParam[ipot] = new NumericFloat<double>(potNames[ipot], initialPotValues[ipot], 0.0, 5.0);
    pList.insert( _potsParam[ipot] );
    _tunePotsParam[ipot] = new CheckValue("Tune", _readOnlyPots & (1<<(ipot+Pds::Uxi::ConfigV2::NumberOfPots)));
    pList.insert( _tunePotsParam[ipot] );
    _roPotsParam[ipot] = new CheckValue("Read Only", _readOnlyPots & (1<<ipot));
    pList.insert( _roPotsParam[ipot] );
  }
}

Pds_ConfigDb::UxiConfig_V2::Private_Data::~Private_Data()
{
  for (unsigned iside=0; iside<Pds::Uxi::ConfigV2::NumberOfSides; iside++) {
    if (_timeOnSideParam[iside]) delete _timeOnSideParam[iside];
    if (_timeOffSideParam[iside]) delete _timeOffSideParam[iside];
    if (_delaySideParam[iside]) delete _delaySideParam[iside];
  }
  for (unsigned ipot=0; ipot<Pds::Uxi::ConfigV2::NumberOfPots; ipot++) {
    if (_potsParam[ipot]) delete _potsParam[ipot];
    if (_tunePotsParam[ipot]) delete _tunePotsParam[ipot];
    if (_roPotsParam[ipot]) delete _roPotsParam[ipot];
  }
}

QLayout* Pds_ConfigDb::UxiConfig_V2::Private_Data::initialize(QWidget* p)
{
  QVBoxLayout* layout = new QVBoxLayout;
  { QVBoxLayout* roi = new QVBoxLayout;
    roi->addWidget(new QLabel("ROI settings: "));
    roi->addLayout(_roiParam.initialize(p));
    roi->addLayout(_concealerROI.add(_firstRowParam  .initialize(p)));
    roi->addLayout(_concealerROI.add(_lastRowParam   .initialize(p)));
    roi->addLayout(_concealerROI.add(_firstFrameParam.initialize(p)));
    roi->addLayout(_concealerROI.add(_lastFrameParam  .initialize(p)));
    roi->setSpacing(5);
    layout->addLayout(roi); }
  { QVBoxLayout* t = new QVBoxLayout;
    t->addWidget(new QLabel("Timing settings: "));
    t->addLayout(_timeOnParam .initialize(p));
    t->addLayout(_timeOffParam.initialize(p));
    t->addLayout(_delayParam  .initialize(p));
    t->setSpacing(5);
    layout->addLayout(_concealerNonExpert.add(t)); }
  for (unsigned iside=0; iside<Pds::Uxi::ConfigV2::NumberOfSides; iside++) {
    { QVBoxLayout* tside = new QVBoxLayout;
      tside->addWidget(new QLabel(sideNames[iside]));
      tside->addLayout(_timeOnSideParam[iside] ->initialize(p));
      tside->addLayout(_timeOffSideParam[iside]->initialize(p));
      tside->addLayout(_delaySideParam[iside]  ->initialize(p));
      tside->setSpacing(5);
      layout->addLayout(_concealerExpert.add(tside)); }
  }
  { QVBoxLayout* r = new QVBoxLayout;
    r->addWidget(new QLabel("Potentiometer settings: "));
    QGridLayout* pgrid = new QGridLayout;
    for (unsigned ipot=0; ipot<Pds::Uxi::ConfigV2::NumberOfPots; ipot++) {
      pgrid->addLayout(_concealerReadOnly[ipot].add(_potsParam[ipot]->initialize(p)), ipot, 0);
      pgrid->addLayout(_concealerReadOnly[ipot].add(_tunePotsParam[ipot]->initialize(p)), ipot, 1);
      pgrid->addLayout(_concealerExpert.add(_roPotsParam[ipot]->initialize(p)), ipot, 2);
    }
    r->addLayout(pgrid);
    r->setSpacing(5);
    layout->addLayout(r); }
  layout->setSpacing(25);

  if (Parameter::allowEdit()) {
    QObject::connect(_roiParam._input, SIGNAL(currentIndexChanged(int)), &_concealerROI, SLOT(show(int)));
  }

  return layout;
}

int Pds_ConfigDb::UxiConfig_V2::Private_Data::pull(void* from) {
  Pds::Uxi::ConfigV2& tc  = *new(from) Pds::Uxi::ConfigV2;
  _width                  = tc.width();
  _height                 = tc.height();
  _numberOfFrames         = tc.numberOfFrames();
  _numberOFBytesPerPixel  = tc.numberOFBytesPerPixel();
  _sensorType             = tc.sensorType();
  _readOnlyPots           = tc.readOnlyPots();

  const Pds::Uxi::RoiCoord roi_rows   = tc.roiRows();
  const Pds::Uxi::RoiCoord roi_frames = tc.roiFrames();
  _roiParam       .value = tc.roiEnable();
  _firstRowParam  .value = roi_rows.first();
  _lastRowParam   .value = roi_rows.last();
  _firstFrameParam.value = roi_frames.first();
  _lastFrameParam .value = roi_frames.last();

  ndarray<const double, 1> pot_data = tc.pots();
  for (unsigned ipot=0; ipot<Pds::Uxi::ConfigV2::NumberOfPots; ipot++) {
    _potsParam[ipot]->value = pot_data[ipot];
    _tunePotsParam[ipot]->value = _readOnlyPots & (1<<(ipot+Pds::Uxi::ConfigV2::NumberOfPots));
    _roPotsParam[ipot]->value = _readOnlyPots & (1<<ipot);
    _concealerReadOnly[ipot].show(!_roPotsParam[ipot]->value || _expert_mode);
  }

  ndarray<const uint32_t, 1> timeon_data = tc.timeOn();
  ndarray<const uint32_t, 1> timeoff_data = tc.timeOff();
  ndarray<const uint32_t, 1> delay_data = tc.delay();
  for (unsigned iside=0; iside<Pds::Uxi::ConfigV2::NumberOfSides; iside++) {
    _timeOnSideParam[iside] ->value = timeon_data[iside];
    _timeOffSideParam[iside]->value = timeoff_data[iside];
    _delaySideParam[iside]  ->value = delay_data[iside];
  }
  _timeOnParam  .value = timeon_data[0];
  _timeOffParam .value = timeoff_data[0];
  _delayParam   .value = delay_data[0];

  _concealerExpert.show(_expert_mode);
  _concealerNonExpert.show(!_expert_mode);
  _concealerROI.show(_roiParam.value);
  
  return sizeof(Pds::Uxi::ConfigV2);
}

int Pds_ConfigDb::UxiConfig_V2::Private_Data::push(void* to) {
  // Gather all the data from the widgets into the correct format
  _readOnlyPots = 0;
  for (unsigned ipot=0; ipot<Pds::Uxi::ConfigV2::NumberOfPots; ipot++) {
    _pots[ipot] = _roPotsParam[ipot]->value ? 0.0 : _potsParam[ipot]->value;
    if (_roPotsParam[ipot]->value)
      _readOnlyPots |= (1<<ipot);
    else if (_tunePotsParam[ipot]->value)
      _readOnlyPots |= (1<<(ipot+Pds::Uxi::ConfigV2::NumberOfPots));
  }

  for (unsigned iside=0; iside<Pds::Uxi::ConfigV2::NumberOfSides; iside++) {
    if (_expert_mode) {
      _timeOn[iside]  = _timeOnSideParam[iside] ->value;
      _timeOff[iside] = _timeOffSideParam[iside]->value;
      _delay[iside]   = _delaySideParam[iside]  ->value;
    } else {
      _timeOn[iside]  = _timeOnParam.value;
      _timeOff[iside] = _timeOffParam.value;
      _delay[iside]   = _delayParam.value;
    }
  }

  new (to) Pds::Uxi::ConfigV2(
    _roiParam.value,
    Pds::Uxi::RoiCoord(_firstRowParam.value, _lastRowParam.value),
    Pds::Uxi::RoiCoord(_firstFrameParam.value, _lastFrameParam.value),
    _width,
    _height,
    _numberOfFrames,
    _numberOFBytesPerPixel,
    _sensorType,
    _timeOn,
    _timeOff,
    _delay,
    _readOnlyPots,
    _pots
  );

  return sizeof(Pds::Uxi::ConfigV2);
}

bool Pds_ConfigDb::UxiConfig_V2::Private_Data::validate()
{
  // Check to see if we are trying to write non-zero values to read only pots
  bool set_read_only = false;
  bool tune_read_only = false;
  unsigned bad_pot = 0;
  double bad_value = 0.0;
  for (unsigned ipot=0; ipot<Pds::Uxi::ConfigV2::NumberOfPots; ipot++) {
    if ((_roPotsParam[ipot]->value) && (_potsParam[ipot]->value != 0.0)) {
      set_read_only = true;
      bad_pot = ipot;
      bad_value = _potsParam[ipot]->value;
      break;
    }
  }

  if (set_read_only) {
    QString msg = QString("Attempting to save the value %1 to the %2 potentiometer, but it is set to read only! Is it okay to ignore this value?").arg(bad_value).arg(potNames[bad_pot]);
    return !QMessageBox::warning(0,"Warning!", msg, "Ignore", "Cancel", 0, 0, 1);
  }

  // Check to see if we trying to tune read only pots
  for (unsigned ipot=0; ipot<Pds::Uxi::ConfigV2::NumberOfPots; ipot++) {
    if ((_roPotsParam[ipot]->value) && (_tunePotsParam[ipot]->value)) {
      tune_read_only = true;
      bad_pot = ipot;
      break;
    }
  }

  if (tune_read_only) {
    QString msg = QString("Attempting to tune the %1 potentiometer, but it is set to read only! Is it okay to ignore this?").arg(potNames[bad_pot]);
    return !QMessageBox::warning(0,"Warning!", msg, "Ignore", "Cancel", 0, 0, 1);
  }

  return true;
}

Pds_ConfigDb::UxiConfig_V2::UxiConfig_V2(bool expert_mode) :
  Serializer("uxi_Config_V2"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::UxiConfig_V2::readParameters (void* from) {
  return _private_data->pull(from);
}

int Pds_ConfigDb::UxiConfig_V2::writeParameters(void* to) {
  return _private_data->push(to);
}

int Pds_ConfigDb::UxiConfig_V2::dataSize() const {
  return _private_data->dataSize();
}

bool Pds_ConfigDb::UxiConfig_V2::validate()
{
  return _private_data->validate();
}

#include "Parameters.icc"
