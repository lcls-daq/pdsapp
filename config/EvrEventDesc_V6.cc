#include "pdsapp/config/EvrEventDesc_V6.hh"
#include "pdsapp/config/EvrConfigType_V6.hh"
#include "pdsdata/psddl/evr.ddl.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QStackedLayout>
#include <QtGui/QStackedWidget>

using namespace Pds_ConfigDb::EvrConfig_V6;

// combo box items
enum { Readout, Command, Transient, Latch };

static const unsigned DescLength = Pds::EvrData::EventCodeV5::DescSize;
EvrEventDesc::EvrEventDesc() :
  _enabled      (false),
  _desc         (NULL, "", DescLength),
  _trans_delay  ("Delay",0, 0, 0x7fffffff),
  _trans_width  ("Duration",1, 1, 0x7fffffff),
  _latch_delay  ("Delay",0, 0, 0x7fffffff),
  _latch_release("Release",0, 0, 255)
{
}

void EvrEventDesc::initialize(QGridLayout* l, unsigned row) 
{
  _enable = new QCheckBox;
  _enable->setChecked(true);

  int column = 0;
  l->addWidget(_enable            , row, column++, Qt::AlignCenter);
  l->addWidget(code_widget()      , row, column++, Qt::AlignCenter);

  _type = new QComboBox;
  _type->addItem("Readout");
  _type->addItem("Command");
  _type->addItem("Control[Transient]");
  _type->addItem("Control[Latch]");
  l->addWidget(_type, row, column++, Qt::AlignCenter);

  l->addLayout(_desc.initialize(0), row, column++, Qt::AlignCenter);
  _desc.widget()->setMaximumWidth(100);

  _stack = new QStackedWidget;
  { QWidget* w = new QWidget(_stack);
    _stack->addWidget(w); }
  { QWidget* w = new QWidget(_stack);
    _stack->addWidget(w); }
  { QWidget* w = new QWidget(_stack);
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addLayout(_trans_delay.initialize(0));
    hl->addLayout(_trans_width.initialize(0));
    w->setLayout(hl);
    _trans_delay.widget()->setMaximumWidth(47);
    _trans_width.widget()->setMaximumWidth(47);
    _stack->addWidget(w); }
  { QWidget* w = new QWidget(_stack);
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addLayout(_latch_delay  .initialize(0));
    hl->addLayout(_latch_release.initialize(0));
    w->setLayout(hl);
    _latch_delay  .widget()->setMaximumWidth(47);
    _latch_release.widget()->setMaximumWidth(47);
    _stack->addWidget(w); }
  l->addWidget(_stack, row, column++, Qt::AlignCenter);

  if (_desc.allowEdit()) {
    ::QObject::connect(_desc._input, SIGNAL(editingFinished()), this, SLOT(update_p()));
    ::QObject::connect(_type, SIGNAL(currentIndexChanged(int)), _stack, SLOT(setCurrentIndex(int)));
    ::QObject::connect(_enable, SIGNAL(toggled(bool)), this, SLOT(enable(bool)));
  }
  else {
    _enable->setEnabled(false);
    _type  ->setEnabled(false);
    ::QObject::connect(_type, SIGNAL(currentIndexChanged(int)), _stack, SLOT(setCurrentIndex(int)));
    ::QObject::connect(_enable, SIGNAL(toggled(bool)), this, SLOT(enable(bool)));
  }
}

bool EvrEventDesc::enabled() const { return _enabled; }

void EvrEventDesc::enable(bool v)
{
  _enabled = v;
  _type         ->setVisible(v);
  _desc.widget()->setVisible(v);
  _stack        ->setVisible(v);
}

const char* EvrEventDesc::get_label() const
{
  return _desc.value;
}

void EvrEventDesc::flush()
{
  _desc.flush();
  _trans_delay.flush();
  _trans_width.flush();
  _latch_delay.flush();
  _latch_release.flush();
  _enable->setChecked( _enabled);
}

void EvrEventDesc::update()
{
  _desc.update();
  _trans_delay.update();
  _trans_width.update();
  _latch_delay.update();
  _latch_release.update();
}

void EvrEventDesc::update_p() { update(); }

void EvrEventDesc::pull(const Pds::EvrData::EventCodeV5& c) 
{
  set_code(c.code());
  strncpy(_desc.value, c.desc(), Pds::EvrData::EventCodeV5::DescSize);
  if      (c.isReadout   ()) {
    _type->setCurrentIndex(Readout);
  }
  else if (c.isCommand   ()) {
    _type->setCurrentIndex(Command);
  }
  else if (c.isLatch     ()) {
    _type->setCurrentIndex(Latch);
    _latch_delay  .value = c.reportDelay();
    _latch_release.value = c.releaseCode();
  }
  else {
    _type->setCurrentIndex(Transient);
    _trans_delay.value = c.reportDelay();
    _trans_width.value = c.reportWidth();
  }
  _enabled = c.code()!=Disabled;
}

void EvrEventDesc::push(Pds::EvrData::EventCodeV5* c) const
{
  uint32_t fill(0);
  switch(_type->currentIndex()) {
  case Readout:
    *new(c) EventCodeType(get_code(),
                          true, false, false,
                          0, 1,
                          fill,fill,fill,
                          _desc.value);
    break;
  case Command:
    *new(c) EventCodeType(get_code(),
                          false, true, false,
                          0, 1,
                          fill,fill,fill,
                          _desc.value);
    break;
  case Transient: // Transient
    *new(c) EventCodeType(get_code(),
                          false, false, false,
                          _trans_delay.value,
                          _trans_width.value,
                          fill,fill,fill,
                          _desc.value);
    break;
  case Latch: // Latch
    *new(c) EventCodeType(get_code(),
                          false, false, true,
                          _latch_delay  .value,
                          _latch_release.value,
                          fill,fill,fill,
                          _desc.value);
    break;
  }
}

void EvrEventDesc::set_enable(bool v) { _enable->setChecked(v); }


