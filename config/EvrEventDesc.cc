#include "pdsapp/config/EvrEventDesc.hh"

#include "pds/config/EvrConfigType.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QStackedLayout>
#include <QtGui/QStackedWidget>

using namespace Pds_ConfigDb;

// combo box items
enum { Readout, Command, Trigger, Transient, Latch };

static const unsigned DescLength = EventCodeType::DescSize;
EvrEventDesc::EvrEventDesc() :
  _enabled      (false),
  _desc         (NULL, "", DescLength),
  _trans_delay  ("Delay",0, 0, 0x7fffffff),
  _trans_width  ("Duration",1, 1, 0x7fffffff),
  _latch_delay  ("Delay",0, 0, 0x7fffffff),
  _latch_release("Release",0, 0, 255),
  _bEnableGroup (false)
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
  _type->addItem("Trigger");
  _type->addItem("Control[Transient]");
  _type->addItem("Control[Latch]");
  l->addWidget(_type, row, column++, Qt::AlignCenter);

  _group = new QComboBox;
  for (int iGroup = 1; iGroup <= EventCodeType::MaxReadoutGroup; ++iGroup)
    _group->addItem(QString().setNum(iGroup));
  _group->setCurrentIndex(0);
  l->addWidget(_group, row, column++, Qt::AlignCenter);
  
  l->addLayout(_desc.initialize(0), row, column++, Qt::AlignCenter);
  _desc.widget()->setMaximumWidth(100);

  _stack = new QStackedWidget;
  _stack->addWidget(new QWidget);
  _stack->addWidget(new QWidget);
  _stack->addWidget(new QWidget);
  { QWidget* w = new QWidget(_stack);
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addLayout(_trans_delay.initialize(0));
    hl->addLayout(_trans_width.initialize(0));
    hl->setContentsMargins(0,0,0,0);
    w->setLayout(hl);
    _trans_delay.widget()->setMaximumWidth(47);
    _trans_width.widget()->setMaximumWidth(47);
    _stack->addWidget(w); }
  { QWidget* w = new QWidget(_stack);
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addLayout(_latch_delay  .initialize(0));
    hl->addLayout(_latch_release.initialize(0));
    hl->setContentsMargins(0,0,0,0);
    w->setLayout(hl);
    _latch_delay  .widget()->setMaximumWidth(47);
    _latch_release.widget()->setMaximumWidth(47);
    _stack->addWidget(w); }
  l->addWidget(_stack, row, column++, Qt::AlignCenter);

  if (_desc.allowEdit()) {
    ::QObject::connect(_desc._input, SIGNAL(editingFinished()), this, SLOT(update_p()));
  }
  else {
    _enable->setEnabled(false);
    _type  ->setEnabled(false);
  }
  ::QObject::connect(_type, SIGNAL(currentIndexChanged(int)), _stack, SLOT(setCurrentIndex(int)));
  ::QObject::connect(_type, SIGNAL(currentIndexChanged(int)), this, SLOT(update_group(int)));
  ::QObject::connect(_enable, SIGNAL(toggled(bool)), this, SLOT(enable(bool)));
}

bool EvrEventDesc::enabled() const { return _enabled; }

void EvrEventDesc::enable(bool v)
{
  _enabled = v;
  _type         ->setVisible(v);
  _desc.widget()->setVisible(v);
  _stack        ->setVisible(v);
  update_group(0);
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

void EvrEventDesc::pull(const EventCodeType& c) 
{
  set_code(c.code());
  strncpy(_desc.value, c.desc(), EventCodeType::DescSize);
  if      (c.isReadout   ()) {
    _type->setCurrentIndex(Readout);
    _group->setCurrentIndex(c.readoutGroup()-1);
  }
  else if (c.isCommand   ()) {
    _type->setCurrentIndex(Command);
  }
  else if (c.isLatch     ()) {
    _type->setCurrentIndex(Latch);
    _latch_delay  .value = c.reportDelay();
    _latch_release.value = c.releaseCode();
  }
  else if (c.readoutGroup()) {
    _type->setCurrentIndex(Trigger);
    _group->setCurrentIndex(c.readoutGroup()-1);
  }
  else {
    _type->setCurrentIndex(Transient);
    _trans_delay.value = c.reportDelay();
    _trans_width.value = c.reportWidth();
  }

  _enabled = c.code()!=Disabled;
  update_group(0);
}

void EvrEventDesc::push(EventCodeType* c) const
{
  uint32_t fill(0);
  switch(_type->currentIndex()) {
  case Readout:
    *new(c) EventCodeType(get_code(),
                          true, false, false,
                          0, 1,
                          fill,fill,fill,
                          _desc.value,
                          1+_group->currentIndex());
    break;
  case Command:
    *new(c) EventCodeType(get_code(),
                          false, true, false,
                          0, 1,
                          fill,fill,fill,
                          _desc.value,
                          0);
    break;
  case Trigger: // Trigger
    *new(c) EventCodeType(get_code(),
                          false, false, false,
                          0, 1,
                          fill,fill,fill,
                          _desc.value,
                          1+_group->currentIndex());
    break;
  case Transient: // Transient
    *new(c) EventCodeType(get_code(),
                          false, false, false,
                          _trans_delay.value,
                          _trans_width.value,
                          fill,fill,fill,
                          _desc.value,
                          0);
    break;
  case Latch: // Latch
    *new(c) EventCodeType(get_code(),
                          false, false, true,
                          _latch_delay  .value,
                          _latch_release.value,
                          fill,fill,fill,
                          _desc.value,
                          0);
    break;
  }
}

void EvrEventDesc::set_enable(bool v) { _enable->setChecked(v); }

void EvrEventDesc::setGroupEnable(bool bEnableGroup)
{
  _bEnableGroup = bEnableGroup;
  update_group(0);
  if (!_bEnableGroup)
    _group->setCurrentIndex(0);
}

void EvrEventDesc::update_group(int)
{
  _group->setVisible(_enabled && _bEnableGroup && 
		     (_type->currentIndex()==Readout ||
		      _type->currentIndex()==Trigger));  
}



