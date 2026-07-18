#include "pdsapp/config/UxiTimingConfig.hh"

#include "pdsapp/config/QtChoiceConcealer.hh"
#include <QtCore/QSignalMapper>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>

const char*   Pds_ConfigDb::UxiTimingConfig::sideNames[] = {
  "A",
  "B",
  NULL,
};

const char*   Pds_ConfigDb::UxiTimingConfig::timingModeNames[] = {
  "Basic",
  "Arbitrary",
  "Manual",
  NULL,
};

using namespace Pds_ConfigDb;

UxiTimingConfig::UxiTimingConfig(bool expert_mode) :
  QObject(),
  _expert_mode(expert_mode),
  _timingModeParam("Timing Mode", UxiConfigType::BasicTiming, timingModeNames)
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
  if (_expert_mode) {
    _pList.insert(&_timingModeParam);
  }
  for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
    _pList.insert(_timeOnSideParam[iside]);
    _pList.insert(_timeOffSideParam[iside]);
    _pList.insert(_delaySideParam[iside]);
    _pList.insert(_manualShutterSideParam[iside]);
    _pList.insert(_arbitraryTimingSideParam[iside]);
  }
}

UxiTimingConfig::~UxiTimingConfig()
{
  for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
    if (_timeOnSideParam[iside]) delete _timeOnSideParam[iside];
    if (_timeOffSideParam[iside]) delete _timeOffSideParam[iside];
    if (_delaySideParam[iside]) delete _delaySideParam[iside];
    if (_manualShutterSideParam[iside]) delete _manualShutterSideParam[iside];
    if (_arbitraryTimingSideParam[iside]) delete _arbitraryTimingSideParam[iside];
  }
}

void UxiTimingConfig::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);
}

QLayout* UxiTimingConfig::initializeLabel(QWidget* p, QSignalMapper* signalMapper, unsigned iside, UxiConfigType::TimingMode mode)
{
  QHBoxLayout* labelLayout = new QHBoxLayout;
  labelLayout->addWidget(new QLabel(QString("Side %1: ").arg(sideNames[iside]),p), 90);
  if (Parameter::allowEdit()) {
    QPushButton* copyButton = new QPushButton(QString("Copy %1 To All").arg(sideNames[iside]), p);
    copyButton->setMinimumWidth(95);
    // connect the copyButton clicked to signalMapper
    QObject::connect(copyButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    // map button to integer signal that encodes the side and timing mode
    signalMapper->setMapping(copyButton, (iside & 0xff) | (mode<<8));
    labelLayout->addWidget(copyButton, 10);
  }
  return labelLayout;
}

QLayout* UxiTimingConfig::initialize(QWidget* p)
{
  QSignalMapper* signalMapper = new QSignalMapper(p);
  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(new QLabel("Timing settings: "));
  if (_expert_mode) {
    layout->addLayout(_timingModeParam.initialize(p));
  }
  for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
    QVBoxLayout* tbasic = new QVBoxLayout;
    tbasic->addLayout(initializeLabel(p, signalMapper, iside, UxiConfigType::BasicTiming));
    tbasic->addLayout(_timeOnSideParam[iside] ->initialize(p));
    tbasic->addLayout(_timeOffSideParam[iside]->initialize(p));
    tbasic->addLayout(_delaySideParam[iside]  ->initialize(p));
    tbasic->setSpacing(5);
    layout->addLayout(_concealerTiming.add(tbasic, 1<<UxiConfigType::BasicTiming));
    QVBoxLayout* tmanual = new QVBoxLayout;
    tmanual->addLayout(initializeLabel(p, signalMapper, iside, UxiConfigType::ManualTiming));
    tmanual->addLayout(_manualShutterSideParam[iside]->initialize(p));
    _manualShutterSideParam[iside]->setWidth(SHUTTER_ITEM_WIDTH);
    tmanual->setSpacing(5);
    layout->addLayout(_concealerTiming.add(tmanual, 1<<UxiConfigType::ManualTiming));
    QVBoxLayout* tarbit = new QVBoxLayout;
    tarbit->addLayout(initializeLabel(p, signalMapper, iside, UxiConfigType::ArbitraryTiming));
    tarbit->addLayout(_arbitraryTimingSideParam[iside]->initialize(p));
    _arbitraryTimingSideParam[iside]->setWidth(TIMING_ITEM_WIDTH);
    tarbit->setSpacing(5);
    layout->addLayout(_concealerTiming.add(tarbit, 1<<UxiConfigType::ArbitraryTiming));
  }
  layout->setSpacing(5);

  if (Parameter::allowEdit()) {
    QObject::connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(copyTiming(int)));
    if (_expert_mode) {
      QObject::connect(_timingModeParam._input, SIGNAL(currentIndexChanged(int)), &_concealerTiming, SLOT(show(int)));
    }
  }

  return layout;
}

void UxiTimingConfig::pull(const UxiConfigType& cfg)
{
  _timingModeParam.value = cfg.timingMode();
  if (_timingModeParam.value == UxiConfigType::BasicTiming) {
    ndarray<const uint32_t, 1> timeon_data = cfg.timeOn();
    ndarray<const uint32_t, 1> timeoff_data = cfg.timeOff();
    ndarray<const uint32_t, 1> delay_data = cfg.delay();

    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      _timeOnSideParam[iside] ->value = timeon_data[iside];
      _timeOffSideParam[iside]->value = timeoff_data[iside];
      _delaySideParam[iside]  ->value = delay_data[iside];
    }
  } else {
    bool manual_timing = _timingModeParam.value == UxiConfigType::ManualTiming;
    unsigned max = cfg.timingLength();
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      ndarray<const uint32_t, 1> timing = cfg.timing(iside);
      for (unsigned idx=0; idx<max; idx++) {
        if (manual_timing) {
          _manualShutterSideParam[iside]->value[idx] = timing[idx];
        } else {
          _arbitraryTimingSideParam[iside]->value[idx] = timing[idx];
        }
      }
    }
  }

  // show only the correct timing mode
  _concealerTiming.show(_timingModeParam.value);
}

UxiConfigType::TimingMode UxiTimingConfig::timingMode() const
{
  return _timingModeParam.value;
}

const uint32_t* UxiTimingConfig::timing()
{
  if (_timingModeParam.value == UxiConfigType::BasicTiming) {
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      _timing[UxiConfigMaxTimingSequence*iside+UxiConfigType::Open]   = _timeOnSideParam[iside] ->value;
      _timing[UxiConfigMaxTimingSequence*iside+UxiConfigType::Closed] = _timeOffSideParam[iside]->value;
      _timing[UxiConfigMaxTimingSequence*iside+UxiConfigType::Delay]  = _delaySideParam[iside]  ->value;
    }
  } else {
    bool manual_timing = _timingModeParam.value == UxiConfigType::ManualTiming;
    unsigned max = manual_timing ? UxiConfigMaxManualShutterSequence : UxiConfigMaxTimingSequence;
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      for (unsigned idx=0; idx<max; idx++) {
        uint32_t value = manual_timing ? _manualShutterSideParam[iside]->value[idx] : _arbitraryTimingSideParam[iside]->value[idx];
        _timing[UxiConfigMaxTimingSequence*iside+idx] = value;
      }
    }
  }

  return _timing;
}

void UxiTimingConfig::flush()
{
  for(Parameter* p=_pList.forward(); p!=_pList.empty(); p=p->forward()) p->flush();
}

void UxiTimingConfig::update()
{
  for(Parameter* p=_pList.forward(); p!=_pList.empty(); p=p->forward()) p->update();
}

void UxiTimingConfig::enable(bool v)
{
  for(Parameter* p=_pList.forward(); p!=_pList.empty(); p=p->forward()) p->enable(v);
}

bool UxiTimingConfig::validate()
{
  bool v = true;
  if (_timingModeParam.value == UxiConfigType::BasicTiming) {
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      uint32_t sum = _timeOnSideParam[iside]->value + _timeOffSideParam[iside]->value + _delaySideParam[iside]->value;
      if (sum > UxiConfigMaxTimingSequence) {
        QString msg = QString("Timing sequence min repeat for side %1 is too long to be implemented: %2 vs. max of %3")
                        .arg(sideNames[iside])
                        .arg(sum)
                        .arg(UxiConfigMaxTimingSequence);
        QMessageBox::critical(0, "Error!", msg);
        v = false;
      }
    }
  } else if (_timingModeParam.value == UxiConfigType::ArbitraryTiming) {
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      uint32_t sum = 0;
      for (unsigned idx=0; idx<UxiConfigMaxTimingSequence; idx++) {
        sum += _arbitraryTimingSideParam[iside]->value[idx];
      }
      if (sum > UxiConfigMaxTimingSequence) {
        QString msg = QString("Timing sequence for side %1 truncated due to length: %2 vs. max of %3")
                        .arg(sideNames[iside])
                        .arg(sum)
                        .arg(UxiConfigMaxTimingSequence);
        v &= !QMessageBox::warning(0,"Warning!", msg, "Ignore", "Cancel", 0, 0, 1);
      }
    }
  } else if (_timingModeParam.value == UxiConfigType::ManualTiming) {
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      uint32_t nround = 0;
      for (unsigned idx=0; idx<UxiConfigMaxManualShutterSequence; idx++) {
        if (_manualShutterSideParam[iside]->value[idx] % SHUTTER_MULTIPLIER != 0) {
          nround++;
        }
      }
      if (nround > 0) {
        QString msg = QString("Timing sequence for side %1 contains %2 value%3 not a multiple of %4")
                        .arg(sideNames[iside])
                        .arg(nround)
                        .arg(nround > 1 ? "s that are" : " that is")
                        .arg(SHUTTER_MULTIPLIER);
        v &= !QMessageBox::warning(0,"Warning!", msg, "Ignore", "Cancel", 0, 0, 1);
      }
    }
  }
  return v;
}

void UxiTimingConfig::copyTiming(int modeside)
{
  unsigned side = modeside & 0xff;
  UxiConfigType::TimingMode mode = static_cast<UxiConfigType::TimingMode>(modeside>>8);
  update();
  if (mode == UxiConfigType::BasicTiming) {
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      if (iside != side) {
        _timeOnSideParam[iside]->value = _timeOnSideParam[side]->value;
        _timeOffSideParam[iside]->value = _timeOffSideParam[side]->value;
        _delaySideParam[iside]->value = _delaySideParam[side]->value;
      }
    }
  } else {
    bool manual_timing = mode == UxiConfigType::ManualTiming;
    unsigned max = manual_timing ? UxiConfigMaxManualShutterSequence : UxiConfigMaxTimingSequence;
    for (unsigned iside=0; iside<UxiConfigNumberOfSides; iside++) {
      if (iside != side) {
        for (unsigned idx=0; idx<max; idx++) {
          if (manual_timing) {
            _manualShutterSideParam[iside]->value[idx] = _manualShutterSideParam[side]->value[idx];
          } else {
            _arbitraryTimingSideParam[iside]->value[idx] = _arbitraryTimingSideParam[side]->value[idx];
          }
        }
      }
    }
  }
  flush();
}

#include "Parameters.icc"
