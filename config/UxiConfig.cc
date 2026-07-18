#include "pdsapp/config/UxiConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pdsapp/config/UxiTimingConfig.hh"
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
  static const uint32_t MAX_ROWS = 1023;
  static const uint32_t MAX_FRAMES = 3;
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
  NumericInt<uint32_t>  _firstRowParam;
  NumericInt<uint32_t>  _lastRowParam;
  NumericInt<uint32_t>  _firstFrameParam;
  NumericInt<uint32_t>  _lastFrameParam;
  NumericFloat<double>* _potsParam[UxiConfigNumberOfPots];
  CheckValue*           _tunePotsParam[UxiConfigNumberOfPots];
  CheckValue*           _roPotsParam[UxiConfigNumberOfPots];
  QtConcealer           _concealerROI;
  QtConcealer           _concealerRegisters;
  QtConcealer           _concealerReadOnly[UxiConfigNumberOfPots];
  UxiTimingConfig       _timingConfig;
  UxiRegisterTable      _registerTable;
private:
  bool _expert_mode;
  uint32_t _width;
  uint32_t _height;
  uint32_t _numberOfFrames;
  uint32_t _numberOFBytesPerPixel;
  uint32_t _sensorType;
  uint32_t _readOnlyPots;
  double   _pots[UxiConfigNumberOfPots];
  static const char*  roiNames[];
  static const char*  oscNames[];
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
  _firstRowParam        ("First Row",             0,  0,  MAX_ROWS),
  _lastRowParam         ("Last Row",              MAX_ROWS,  0,  MAX_ROWS),
  _firstFrameParam      ("First Frame",           0,  0,  MAX_FRAMES),
  _lastFrameParam       ("Last Frame",            MAX_FRAMES,  0,  MAX_FRAMES),
  _timingConfig         (expert_mode),
  _registerTable        (expert_mode),
  _expert_mode          (expert_mode),
  _width                (512),
  _height               (1024),
  _numberOfFrames       (2),
  _numberOFBytesPerPixel(2),
  _sensorType           (1),
  _readOnlyPots         (8111)
{
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
  _timingConfig.insert(pList);
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
  layout->addLayout(_timingConfig.initialize(p));
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

  ndarray<const double, 1> pot_data = tc.pots();
  for (unsigned ipot=0; ipot<UxiConfigNumberOfPots; ipot++) {
    _potsParam[ipot]->value = pot_data[ipot];
    _tunePotsParam[ipot]->value = _readOnlyPots & (1<<(ipot+UxiConfigNumberOfPots));
    _roPotsParam[ipot]->value = _readOnlyPots & (1<<ipot);
    _concealerReadOnly[ipot].show(!_roPotsParam[ipot]->value || _expert_mode);
  }

  // load timing values into the timing config widget
  _timingConfig.pull(tc);
  // load register values into the register table
  _registerTable.pull(tc);

  _concealerROI.show(_roiParam.value);
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
    _timingConfig.timingMode(),
    _timingConfig.timing(),
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

  return _timingConfig.validate() && _registerTable.validate();
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
