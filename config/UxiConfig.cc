#include "pdsapp/config/UxiConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pdsapp/config/QtChoiceConcealer.hh"
#include "pdsapp/config/UxiRegisterConfig.hh"
#include "pds/config/UxiConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>

#include <math.h>
#include <new>

using namespace Pds_ConfigDb;

class Pds_ConfigDb::UxiConfig::Private_Data : public Parameter {
  static const int SHUTTER_ITEM_WIDTH = 90;
  static const int TIMING_ITEM_WIDTH = 40;
  static const uint32_t MAX_ROWS = 1023;
  static const uint32_t MAX_FRAMES = 3;
  static const uint32_t MIN_SHUTTER_TIME = 75;
  static const uint32_t MAX_SHUTTER_TIME = 2000000000;
public:
  Private_Data(bool expert_mode);
  ~Private_Data();

   QLayout* initialize( QWidget* p );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { UxiConfigType cfg(_registerTable.nregisters()); return cfg._sizeof(); }
   void flush ()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
   void update()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
   void enable(bool l)
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }
   bool   validate();

  Pds::LinkedList<Parameter> pList;
  Enumerated<UxiConfigType::RoiMode>    _roiParam;
  Enumerated<UxiConfigType::OscMode>    _oscParam;
  Enumerated<UxiConfigType::TimingMode> _timingModeParam;
  NumericInt<uint32_t>  _firstRowParam;
  NumericInt<uint32_t>  _lastRowParam;
  NumericInt<uint32_t>  _firstFrameParam;
  NumericInt<uint32_t>  _lastFrameParam;
  NumericInt<uint32_t>  _timeOnParam;
  NumericInt<uint32_t>  _timeOffParam;
  NumericInt<uint32_t>  _delayParam;
  NumericInt<uint32_t>* _timeOnSideParam[UxiConfigNumberOfSides];
  NumericInt<uint32_t>* _timeOffSideParam[UxiConfigNumberOfSides];
  NumericInt<uint32_t>* _delaySideParam[UxiConfigNumberOfSides];
  NumericIntArray<uint32_t, UxiConfigMaxManualShutterSequence>  _manualShutterParam;
  NumericIntArray<uint32_t, UxiConfigMaxManualShutterSequence>* _manualShutterSideParam[UxiConfigNumberOfSides];
  NumericIntArray<uint32_t, UxiConfigMaxTimingSequence>  _arbitraryTimingParam;
  NumericIntArray<uint32_t, UxiConfigMaxTimingSequence>* _arbitraryTimingSideParam[UxiConfigNumberOfSides];
  NumericFloat<double>* _potsParam[UxiConfigNumberOfPots];
  CheckValue*           _tunePotsParam[UxiConfigNumberOfPots];
  CheckValue*           _roPotsParam[UxiConfigNumberOfPots];
  QtConcealer           _concealerROI;
  QtConcealer           _concealerRegisters;
  QtChoiceConcealer     _concealerTiming;
  QtConcealer           _concealerReadOnly[UxiConfigNumberOfPots];
  UxiRegisterTable      _registerTable;
private:
  bool _expert_mode;
  uint32_t _width;
  uint32_t _height;
  uint32_t _numberOfFrames;
  uint32_t _numberOFBytesPerPixel;
  uint32_t _sensorType;
  uint32_t _timing[UxiConfigNumberOfSides * UxiConfigMaxTimingSequence];
  uint32_t _readOnlyPots;
  double   _pots[UxiConfigNumberOfPots];
  static const char*  roiNames[];
  static const char*  oscNames[];
  static const char*  sideNames[];
  static const char*  timingModeNames[];
  static const char*  potNames[];
  static const double initialPotValues[UxiConfigNumberOfPots];
};

const char*   Pds_ConfigDb::UxiConfig::Private_Data::roiNames[] = {
  "Off",
  "On",
  NULL,
};

const char*   Pds_ConfigDb::UxiConfig::Private_Data::oscNames[] = {
  "RelaxationOsc",
  "RingOscWithCaps",
  "RingOscNoCaps",
  "ExternalClock",
  NULL,
};

const char*   Pds_ConfigDb::UxiConfig::Private_Data::sideNames[] = {
  "Side A: ",
  "Side B: ",
  NULL,
};

const char*   Pds_ConfigDb::UxiConfig::Private_Data::timingModeNames[] = {
  "Basic",
  "Arbitrary",
  "Manual",
  NULL,
};

const char*   Pds_ConfigDb::UxiConfig::Private_Data::potNames[] = {
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
const double  Pds_ConfigDb::UxiConfig::Private_Data::initialPotValues[] = {
  0.0, 0.0, 0.0, 0.0, 2.8, 0.0, 1.53, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};


Pds_ConfigDb::UxiConfig::Private_Data::Private_Data(bool expert_mode) :
  _roiParam             ("ROI Mode",              UxiConfigType::Off, roiNames),
  _oscParam             ("Oscillator Mode",       UxiConfigType::RelaxationOsc, oscNames),
  _timingModeParam      ("Timing Mode",           UxiConfigType::BasicTiming, timingModeNames),
  _firstRowParam        ("First Row",             0,  0,  MAX_ROWS),
  _lastRowParam         ("Last Row",              0,  0,  MAX_ROWS),
  _firstFrameParam      ("First Frame",           0,  0,  MAX_FRAMES),
  _lastFrameParam       ("Last Frame",            0,  0,  MAX_FRAMES),
  _timeOnParam          ("Time On",               2,  0,  UxiConfigMaxTimingSequence),
  _timeOffParam         ("Time Off",              2,  0,  UxiConfigMaxTimingSequence),
  _delayParam           ("Initial delay (ns)",    0,  0,  UxiConfigMaxTimingSequence),
  _manualShutterParam   ("Manual Shutters (ns)",  MIN_SHUTTER_TIME, MIN_SHUTTER_TIME, MAX_SHUTTER_TIME),
  _arbitraryTimingParam (NULL,                    0,  0,  UxiConfigMaxTimingSequence, Decimal, 1., 4),
  _registerTable        (expert_mode),
  _expert_mode          (expert_mode),
  _width                (512),
  _height               (1024),
  _numberOfFrames       (2),
  _numberOFBytesPerPixel(2),
  _sensorType           (1),
  _readOnlyPots         (8111)
{
  for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
    _timeOnSideParam[iside] = new NumericInt<uint32_t>("Time On", 2, 0, UxiConfigMaxTimingSequence);
    _timeOffSideParam[iside] = new NumericInt<uint32_t>("Time Off", 2, 0, UxiConfigMaxTimingSequence);
    _delaySideParam[iside] = new NumericInt<uint32_t>("Initial delay (ns)", 0, 0, UxiConfigMaxTimingSequence);
    _manualShutterSideParam[iside] = new NumericIntArray<uint32_t, UxiConfigMaxManualShutterSequence>("Manual Shutters (ns)", 
                                                                                                      MIN_SHUTTER_TIME,
                                                                                                      MIN_SHUTTER_TIME,
                                                                                                      MAX_SHUTTER_TIME);
    _arbitraryTimingSideParam[iside] = new NumericIntArray<uint32_t, UxiConfigMaxTimingSequence>(NULL,
                                                                                                 0,
                                                                                                 0,
                                                                                                 UxiConfigMaxTimingSequence,
                                                                                                 Decimal,
                                                                                                 1.,
                                                                                                 4);
  }
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
    _potsParam[ipot] = new NumericFloat<double>(potNames[ipot], initialPotValues[ipot], 0.0, 5.0);
    _tunePotsParam[ipot] = new CheckValue("Tune", _readOnlyPots & (1<<(ipot+UxiConfigNumberOfPots)));
    _roPotsParam[ipot] = new CheckValue("Read Only", _readOnlyPots & (1<<ipot));
  }

  pList.insert( &_roiParam );
  pList.insert( &_oscParam );
  pList.insert( &_firstRowParam );
  pList.insert( &_lastRowParam );
  pList.insert( &_firstFrameParam );
  pList.insert( &_lastFrameParam );
  if (!_expert_mode) {
    pList.insert( &_timeOnParam );
    pList.insert( &_timeOffParam );
    pList.insert( &_delayParam );
    pList.insert( &_manualShutterParam );
    pList.insert( &_arbitraryTimingParam );
  } else {
    pList.insert( &_timingModeParam );
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      pList.insert( _timeOnSideParam[iside] );
      pList.insert( _timeOffSideParam[iside] );
      pList.insert( _delaySideParam[iside] );
      pList.insert( _manualShutterSideParam[iside] );
      pList.insert( _arbitraryTimingSideParam[iside] );
    }
  }
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
    pList.insert( _potsParam[ipot] );
    pList.insert( _tunePotsParam[ipot] );
    if (_expert_mode) {
      pList.insert( _roPotsParam[ipot] );
    }
  }
  _registerTable.insert(pList);
}

Pds_ConfigDb::UxiConfig::Private_Data::~Private_Data()
{
  for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
    if (_timeOnSideParam[iside]) delete _timeOnSideParam[iside];
    if (_timeOffSideParam[iside]) delete _timeOffSideParam[iside];
    if (_delaySideParam[iside]) delete _delaySideParam[iside];
    if (_manualShutterSideParam[iside]) delete _manualShutterSideParam[iside];
    if (_arbitraryTimingSideParam[iside]) delete _arbitraryTimingSideParam[iside];
  }
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
    if (_potsParam[ipot]) delete _potsParam[ipot];
    if (_tunePotsParam[ipot]) delete _tunePotsParam[ipot];
    if (_roPotsParam[ipot]) delete _roPotsParam[ipot];
  }
}

QLayout* Pds_ConfigDb::UxiConfig::Private_Data::initialize(QWidget* p)
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
  { QVBoxLayout* osc = new QVBoxLayout;
    osc->addWidget(new QLabel("Oscillator settings: "));
    osc->addLayout(_oscParam.initialize(p));
    osc->setSpacing(5);
    layout->addLayout(osc); }
  { QVBoxLayout* t = new QVBoxLayout;
    t->addWidget(new QLabel("Timing settings: "));
    if (!_expert_mode) {
      QVBoxLayout* tb = new QVBoxLayout;
      tb->addLayout(_timeOnParam    .initialize(p));
      tb->addLayout(_timeOffParam   .initialize(p));
      tb->addLayout(_delayParam     .initialize(p));
      tb->setSpacing(5);
      t->addLayout(_concealerTiming.add(tb, 1<<UxiConfigType::BasicTiming));
      QVBoxLayout* tm = new QVBoxLayout;
      tm->addLayout(_manualShutterParam.initialize(p)); _manualShutterParam.setWidth(SHUTTER_ITEM_WIDTH);
      tm->setSpacing(5);
      t->addLayout(_concealerTiming.add(tm, 1<<UxiConfigType::ManualTiming));
      QVBoxLayout* ta = new QVBoxLayout;
      ta->addLayout(_arbitraryTimingParam.initialize(p)); _arbitraryTimingParam.setWidth(TIMING_ITEM_WIDTH);
      ta->setSpacing(5);
      t->addLayout(_concealerTiming.add(ta, 1<<UxiConfigType::ArbitraryTiming));
    } else {
      t->addLayout(_timingModeParam.initialize(p));
      for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
        QVBoxLayout* tside = new QVBoxLayout;
        tside->addWidget(new QLabel(sideNames[iside]));
        tside->addLayout(_timeOnSideParam[iside] ->initialize(p));
        tside->addLayout(_timeOffSideParam[iside]->initialize(p));
        tside->addLayout(_delaySideParam[iside]  ->initialize(p));
        tside->setSpacing(5);
        t->addLayout(_concealerTiming.add(tside, 1<<UxiConfigType::BasicTiming));
        QVBoxLayout* tmside = new QVBoxLayout;
        tmside->addWidget(new QLabel(sideNames[iside]));
        tmside->addLayout(_manualShutterSideParam[iside]->initialize(p)); _manualShutterSideParam[iside]->setWidth(SHUTTER_ITEM_WIDTH);
        tmside->setSpacing(5);
        t->addLayout(_concealerTiming.add(tmside, 1<<UxiConfigType::ManualTiming));
        QVBoxLayout* taside = new QVBoxLayout;
        taside->addWidget(new QLabel(sideNames[iside]));
        taside->addLayout(_arbitraryTimingSideParam[iside]->initialize(p)); _arbitraryTimingSideParam[iside]->setWidth(TIMING_ITEM_WIDTH);
        taside->setSpacing(5);
        t->addLayout(_concealerTiming.add(taside, 1<<UxiConfigType::ArbitraryTiming));
      }
    }
    t->setSpacing(5);
    layout->addLayout(t); }
  { QVBoxLayout* r = new QVBoxLayout;
    r->addWidget(new QLabel("Potentiometer settings: "));
    QGridLayout* pgrid = new QGridLayout;
    for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
      pgrid->addLayout(_concealerReadOnly[ipot].add(_potsParam[ipot]->initialize(p)), ipot, 0);
      pgrid->addLayout(_concealerReadOnly[ipot].add(_tunePotsParam[ipot]->initialize(p)), ipot, 1);
      if (_expert_mode) {
        pgrid->addLayout(_roPotsParam[ipot]->initialize(p), ipot, 2);
      }
    }
    r->addLayout(pgrid);
    r->setSpacing(5);
    layout->addLayout(r); }
  layout->addLayout(_concealerRegisters.add(_registerTable.initialize(p)));
  layout->setSpacing(25);

  if (Parameter::allowEdit()) {
    QObject::connect(_roiParam._input, SIGNAL(currentIndexChanged(int)), &_concealerROI, SLOT(show(int)));
    if (_expert_mode) {
      QObject::connect(_timingModeParam._input, SIGNAL(currentIndexChanged(int)), &_concealerTiming, SLOT(show(int)));
    }
  }

  return layout;
}

int Pds_ConfigDb::UxiConfig::Private_Data::pull(void* from) {
  UxiConfigType& tc = *new(from) UxiConfigType;
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

  _oscParam       .value = tc.oscillator();
  _timingModeParam.value = tc.timingMode();

  ndarray<const double, 1> pot_data = tc.pots();
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
    _potsParam[ipot]->value = pot_data[ipot];
    _tunePotsParam[ipot]->value = _readOnlyPots & (1<<(ipot+UxiConfigNumberOfPots));
    _roPotsParam[ipot]->value = _readOnlyPots & (1<<ipot);
    _concealerReadOnly[ipot].show(!_roPotsParam[ipot]->value || _expert_mode);
  }

  if (_timingModeParam.value == UxiConfigType::BasicTiming) {
    ndarray<const uint32_t, 1> timeon_data = tc.timeOn();
    ndarray<const uint32_t, 1> timeoff_data = tc.timeOff();
    ndarray<const uint32_t, 1> delay_data = tc.delay();

    if (_expert_mode) {
      for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
        _timeOnSideParam[iside] ->value = timeon_data[iside];
        _timeOffSideParam[iside]->value = timeoff_data[iside];
        _delaySideParam[iside]  ->value = delay_data[iside];
      }
    } else {
      _timeOnParam  .value = timeon_data[0];
      _timeOffParam .value = timeoff_data[0];
      _delayParam   .value = delay_data[0];
    }
  } else {
    bool manual_timing = _timingModeParam.value == UxiConfigType::ManualTiming;
    unsigned max = manual_timing ? UxiConfigMaxManualShutterSequence : UxiConfigMaxTimingSequence;
    if (_expert_mode) {
      for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
        ndarray<const uint32_t, 1> timing = tc.timing(iside);
        for (unsigned idx=0; idx<max; idx++) {
          if (manual_timing) {
            _manualShutterSideParam[iside]->value[idx] = timing[idx];
          } else {
            _arbitraryTimingSideParam[iside]->value[idx] = timing[idx];
          }
        }
      }
    } else {
      ndarray<const uint32_t, 1> timing = tc.timing(0);
      for (unsigned idx=0; idx<max; idx++) {
        if (manual_timing) {
          _manualShutterParam.value[idx] = timing[idx];
        } else {
          _arbitraryTimingParam.value[idx] = timing[idx];
        }
      }
    }
  }
  _concealerROI.show(_roiParam.value);
  _concealerTiming.show(_timingModeParam.value);

  // load register values into the register table
  _registerTable.pull(tc);

  // always show in expert or in user mode if more than zero registers
  _concealerRegisters.show(_expert_mode || _registerTable.nregisters() > 0);
  
  return tc._sizeof();
}

int Pds_ConfigDb::UxiConfig::Private_Data::push(void* to) {
  // Gather all the data from the widgets into the correct format
  _readOnlyPots = 0;
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
    _pots[ipot] = _roPotsParam[ipot]->value ? 0.0 : _potsParam[ipot]->value;
    if (_roPotsParam[ipot]->value)
      _readOnlyPots |= (1<<ipot);
    else if (_tunePotsParam[ipot]->value)
      _readOnlyPots |= (1<<(ipot+UxiConfigNumberOfPots));
  }

  if (_timingModeParam.value == UxiConfigType::BasicTiming) {
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      if (_expert_mode) {
        _timing[UxiConfigMaxTimingSequence*iside]   = _timeOnSideParam[iside] ->value;
        _timing[UxiConfigMaxTimingSequence*iside+1] = _timeOffSideParam[iside]->value;
        _timing[UxiConfigMaxTimingSequence*iside+2] = _delaySideParam[iside]  ->value;
      } else {
        _timing[UxiConfigMaxTimingSequence*iside]   = _timeOnParam.value;
        _timing[UxiConfigMaxTimingSequence*iside+1] = _timeOffParam.value;
        _timing[UxiConfigMaxTimingSequence*iside+2] = _delayParam.value;
      }
    }
  } else {
    bool manual_timing = _timingModeParam.value == UxiConfigType::ManualTiming;
    unsigned max = manual_timing ? UxiConfigMaxManualShutterSequence : UxiConfigMaxTimingSequence;
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      for (unsigned idx=0; idx<max; idx++) {
        uint32_t value = 0;
        if (_expert_mode) {
          value = manual_timing ? _manualShutterSideParam[iside]->value[idx] : _arbitraryTimingSideParam[iside]->value[idx];
        } else {
          value = manual_timing ? _manualShutterParam.value[idx] : _arbitraryTimingParam.value[idx];
        }
        _timing[UxiConfigMaxTimingSequence*iside+idx] = value;
      }
    }
  }

  // get register names and values from the register table
  Pds::Uxi::Register* registers = new Pds::Uxi::Register[_registerTable.nregisters()];
  _registerTable.push(registers);

  UxiConfigType& cfg = *new (to) UxiConfigType(
    _roiParam.value,
    Pds::Uxi::RoiCoord(_firstRowParam.value, _lastRowParam.value),
    Pds::Uxi::RoiCoord(_firstFrameParam.value, _lastFrameParam.value),
    _oscParam.value,
    _width,
    _height,
    _numberOfFrames,
    _numberOFBytesPerPixel,
    _sensorType,
    _timingModeParam.value,
    _timing,
    _readOnlyPots,
    _pots,
    _registerTable.nregisters(),
    registers
  );

  delete[] registers;

  return cfg._sizeof();
}

bool Pds_ConfigDb::UxiConfig::Private_Data::validate()
{
  // Check to see if we are trying to write non-zero values to read only pots
  bool set_read_only = false;
  bool tune_read_only = false;
  unsigned bad_pot = 0;
  double bad_value = 0.0;
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
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
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
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

  return _registerTable.validate();
}

Pds_ConfigDb::UxiConfig::UxiConfig(bool expert_mode) :
  Serializer("uxi_Config"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::UxiConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int Pds_ConfigDb::UxiConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int Pds_ConfigDb::UxiConfig::dataSize() const {
  return _private_data->dataSize();
}

bool Pds_ConfigDb::UxiConfig::validate()
{
  return _private_data->validate();
}

#include "Parameters.icc"
