#include "pdsapp/config/EvrPulseTable.hh"

#include "EvrEventCode.hh"
#include "EvrPulseConfig.hh"
#include "EvrOutputMap.hh"
#include "pdsapp/config/QrLabel.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pds/config/EvrIOConfigType.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

static const int PolarityGroup = 100;

namespace Pds_ConfigDb
{
  class PolarityButton : public QPushButton {
  public:
    enum State { None, Pos, Neg, NumberOfStates };
    PolarityButton(State s = None) :
      QPushButton("None")
    {
      setMaximumWidth(47);
      setCheckable(true); 
      setState(s);
    }
  public:
    void setState(State s)
    {
      _state = s;
      switch(_state) {
      case None : setText("None");  break;
      case Pos  : setText("Pos" );  break;
      case Neg  : setText("Neg" );  break;
      default  :	break;
      }
      emit toggled(_state != None);
    }
    void nextCheckState() 
    {
      setState(State((_state+1)%NumberOfStates));
    }
    State state() const { return _state; }
  private:
    State _state;
  };


  static const double EvrPeriod = 1./119e6;

  class Pulse {
  public:
    Pulse() :
      _eventCode( NumericInt<unsigned>(NULL,0,0, 255, Decimal)),
      _delay    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvrPeriod)),
      _width    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvrPeriod)),
      _rdelay   ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Decimal)),
      _rwidth   ( NumericInt<unsigned>(NULL,1,1, 0x7fffffff, Decimal))
    {}
  public:
    void enable(bool v) 
    {
      bool allowEdit = Parameter::allowEdit();
      _eventCode.enable(v);
      _eventCode.widget()->setVisible(v);
      _rdelay   .enable(v);
      _rdelay   .widget()->setVisible(v);
      _rwidth   .enable(v);
      _rwidth   .widget()->setVisible(v);
      _polarity  ->setEnabled(v && allowEdit);
      _polarity  ->setVisible(v);
      _readout   ->setEnabled(v && allowEdit);
      _readout   ->setVisible(v);
      _terminator->setEnabled(v && allowEdit);
      _terminator->setVisible(v);
      v &= (_polarity->state()!=PolarityButton::None);
      _delay    .enable(v);
      _width    .enable(v);
      for(unsigned i=0; i<EvrPulseTable::MaxOutputs; i++) {
	_outputs[i]->setEnabled(v && allowEdit);
	_outputs[i]->setVisible(v);
      }
    }
    void reset () 
    {
      _eventCode.value = 0; 
      _delay    .value = 0;
      _width    .value = 0;
      _rdelay   .value = 0;
      _rwidth   .value = 1;
      _polarity  ->setState(PolarityButton::None);
      _readout   ->setChecked(false);
      _terminator->setChecked(false);
      for(unsigned i=0; i<EvrPulseTable::MaxOutputs; i++)
	_outputs[i]->setChecked(false);
    }
    void set   (const Pulse& p) 
    {
      _eventCode.value = p._eventCode.value;
      _rdelay   .value = p._rdelay   .value;
      _rwidth   .value = p._rwidth   .value;
      _readout   ->setChecked(p._readout   ->isChecked());
      _terminator->setChecked(p._terminator->isChecked());
    }
  public:
    void initialize(QWidget* parent,
		    QGridLayout* layout, 
		    int row, int bid,
		    QButtonGroup* egroup,
		    QButtonGroup* tgroup,
		    QButtonGroup* ogroup) 
    {
      _enable    = new QCheckBox;
      _polarity  = new PolarityButton;
      for(unsigned i=0; i<10; i++)
	_outputs[i] = new QCheckBox;
      _readout    = new QCheckBox;
      _terminator = new QCheckBox;

      egroup->addButton(_enable,bid);
      egroup->addButton(_polarity,bid+PolarityGroup);
      for(unsigned i=0; i<10; i++)
	ogroup->addButton(_outputs[i],bid*10+i);
      tgroup->addButton(_terminator,bid);

      int column = 0;
      layout->addWidget(_enable, row, column++, Qt::AlignCenter);
      layout->addLayout(_eventCode.initialize(parent), row, column++, Qt::AlignCenter);
      layout->addWidget(_polarity , row, column++, Qt::AlignCenter);
      layout->setColumnMinimumWidth(column,97);
      layout->addLayout(_delay.initialize(parent)    , row, column++, Qt::AlignCenter);
      layout->setColumnMinimumWidth(column,97);
      layout->addLayout(_width.initialize(parent)    , row, column++, Qt::AlignCenter);
      for(unsigned i=0; i<10; i++)
	layout->addWidget(_outputs[i], row, column++, Qt::AlignCenter);
      layout->addLayout(_rdelay.initialize(parent)    , row, column++, Qt::AlignCenter);
      layout->addLayout(_rwidth.initialize(parent)    , row, column++, Qt::AlignCenter);
      layout->addWidget(_readout   , row, column++, Qt::AlignCenter);
      layout->addWidget(_terminator, row, column++, Qt::AlignCenter);

      _eventCode.widget()->setMaximumWidth(47);
      _delay    .widget()->setMaximumWidth(97);
      _width    .widget()->setMaximumWidth(97);
      _rdelay   .widget()->setMaximumWidth(47);
      _rwidth   .widget()->setMaximumWidth(47);

      ::QObject::connect(_polarity, SIGNAL(toggled(bool)), _delay.widget(), SLOT(setVisible(bool)));
      ::QObject::connect(_polarity, SIGNAL(toggled(bool)), _width.widget(), SLOT(setVisible(bool)));
      for(unsigned i=0; i<10; i++)
	::QObject::connect(_polarity, SIGNAL(toggled(bool)), _outputs[i], SLOT(setVisible(bool)));

      _enable->setEnabled(Parameter::allowEdit());

      reset();
      enable(false);
    }

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_eventCode);
      pList.insert(&_delay);
      pList.insert(&_width);
      pList.insert(&_rdelay);
      pList.insert(&_rwidth);
    }

  public:
    QCheckBox*            _enable;
    NumericInt<unsigned>  _eventCode;
    PolarityButton*       _polarity;
    NumericInt<unsigned>  _delay;
    NumericInt<unsigned>  _width;
    QCheckBox*            _outputs[EvrPulseTable::MaxOutputs];
    QCheckBox*            _readout;
    QCheckBox*            _terminator;
    NumericInt<unsigned>  _rdelay;
    NumericInt<unsigned>  _rwidth;
  };
};

using namespace Pds_ConfigDb;

EvrPulseTable::EvrPulseTable(const EvrConfig& c) : 
  Parameter(NULL),
  _cfg(c)
{
  for(unsigned i=0; i<MaxPulses; i++)
    _pulses[i] = new Pulse;
}

EvrPulseTable::~EvrPulseTable()
{
}

void EvrPulseTable::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);
  
  for(unsigned i=0; i<MaxPulses; i++)
    _pulses[i]->insert(_pList);
}

int EvrPulseTable::pull(const void* from) {
  const EvrConfigType& tc = *reinterpret_cast<const EvrConfigType*>(from);
  unsigned npulses = 0;
  for(unsigned i=0; i<tc.neventcodes(); i++) {
    const EvrConfigType::EventCodeType& ec = tc.eventcode(i);
    unsigned m = ec.maskTrigger();
    if (m==0) {
      Pulse& p = *_pulses[npulses];
      p._enable    ->setChecked(true);
      update_enable(npulses);
      p._eventCode  .value = ec.code();
      p._readout   ->setChecked(ec.isReadout());
      p._terminator->setChecked(ec.isTerminator());
      p._rdelay     .value = ec.reportDelay();
      p._rwidth     .value = ec.reportWidth();
      p._polarity  ->setState(PolarityButton::None);
      npulses++;
    }
    else {
      for(unsigned j=0; j<tc.npulses(); j++) {
	if (m & (1<<j)) {
	  Pulse& p = *_pulses[npulses];
	  p._enable    ->setChecked(true);
	  update_enable(npulses);
	  p._eventCode  .value = ec.code();
	  p._readout   ->setChecked(ec.isReadout());
	  p._terminator->setChecked(ec.isTerminator());
	  p._rdelay     .value = ec.reportDelay();
	  p._rwidth     .value = ec.reportWidth();
	  const EvrConfigType::PulseType& pt = tc.pulse(j);
	  p._polarity  ->setState(pt.polarity()==Pds_ConfigDb::Enums::Pos ? 
				  PolarityButton::Pos : PolarityButton::Neg);
	  p._delay      .value = pt.delay();
	  p._width      .value = pt.width();
	  for(unsigned k=0; k<MaxOutputs; k++)
	    p._outputs[k]->setChecked(false);
	  for(unsigned k=0; k<tc.noutputs(); k++) {
	    const EvrConfigType::OutputMapType& om = tc.output_map(k);
	    if ( om.source()==EvrConfigType::OutputMapType::Pulse &&
		 om.source_id()==j )
	      p._outputs[om.conn_id()]->setChecked(true);
				       
	  }
	  npulses++;
	}
      }
    }
  }
  while(npulses < MaxPulses) {
    _pulses[npulses]->_enable->setChecked(false);
    update_enable(npulses++);
  }

  update_eventcode();

  return tc.size();
}

int EvrPulseTable::push(void* to) const {

  unsigned nom = 0;
  EvrConfigType::OutputMapType* om = new EvrConfigType::OutputMapType[MaxOutputs];

  unsigned npt = 0;
  EvrConfigType::PulseType*     pt = new EvrConfigType::PulseType    [MaxPulses];

  for(unsigned i=0; i<MaxPulses; i++) {
    const Pulse& p = *_pulses[i];
    if (!p._enable->isChecked())
      continue;

    if (p._polarity->state() == PolarityButton::None)
      continue;

    *new(&pt[npt]) EvrConfigType::PulseType(npt, 
					    p._polarity->state() == PolarityButton::Pos ? 0 : 1,
					    1,
					    p._delay.value,
					    p._width.value);
    for(unsigned j=0; j<MaxOutputs; j++) {
      if (p._outputs[j]->isChecked())
	*new(&om[nom++]) EvrConfigType::OutputMapType( EvrConfigType::OutputMapType::Pulse, npt,
						       EvrConfigType::OutputMapType::UnivIO, j );
    }
    npt++;
  }

  unsigned nec = 0;
  EvrConfigType::EventCodeType* ec = new EvrConfigType::EventCodeType[MaxPulses];

  unsigned pulseMask = 1;
  for(unsigned i=0; i<MaxPulses; i++) {
    const Pulse& p = *_pulses[i];
    if (!p._enable->isChecked())
      continue;

    unsigned pm = 0;
    if (p._polarity->state() != PolarityButton::None) {
      pm = pulseMask;
      pulseMask <<= 1;
    }

    bool lunique = true;
    for(unsigned i=0; i<nec; i++)
      if (ec[i].code() == p._eventCode.value) {
	*new(&ec[i]) EvrConfigType::EventCodeType(ec[i].code(),
						  ec[i].isReadout(),
						  ec[i].isTerminator(),
						  ec[i].reportDelay(),
						  ec[i].reportWidth(),
						  ec[i].maskTrigger() | pm,
						  0, 
						  0);
	lunique = false;
	break;
      }
    if (lunique)
      *new(&ec[nec++]) EvrConfigType::EventCodeType(p._eventCode.value,
						    p._readout->isChecked(),
						    p._terminator->isChecked(),
						    p._rdelay.value,
						    p._rwidth.value,
						    pm,
						    0, 
						    0);
  }  

  EvrConfigType& tc = *new(to) EvrConfigType(nec, ec,
					     npt, pt,
					     nom, om);

  delete[] ec;
  delete[] pt;
  delete[] om;

  return tc.size();
}

int EvrPulseTable::dataSize() const {

  unsigned nom = 0;
  for(unsigned i=0; i<MaxPulses; i++)
    if (_pulses[i]->_enable->isChecked())
      for(unsigned j=0; j<MaxOutputs; j++)
	if (_pulses[i]->_outputs[j]->isChecked())
	  nom++;

  unsigned npt = 0;

  for(unsigned i=0; i<MaxPulses; i++) {
    const Pulse& p = *_pulses[i];
    if (p._polarity->state() == PolarityButton::None)
      continue;
    npt++;
  }

  unsigned nec = 0;
  EvrConfigType::EventCodeType* ec = new EvrConfigType::EventCodeType[MaxPulses];

  for(unsigned i=0; i<MaxPulses; i++) {
    const Pulse& p = *_pulses[i];
    if (!_pulses[i]->_enable->isChecked())
      continue;

    bool lunique = true;
    for(unsigned i=0; i<nec; i++)
      if (ec[i].code() == p._eventCode.value) {
	*new(&ec[i]) EvrConfigType::EventCodeType(ec[i].code(),
						  ec[i].isReadout(),
						  ec[i].isTerminator(),
						  ec[i].reportDelay(),
						  ec[i].reportWidth(),
						  0,
						  0, 
						  0);
	lunique = false;
	break;
      }
    if (lunique)
      *new(&ec[nec++]) EvrConfigType::EventCodeType(p._eventCode.value,
						    p._readout->isChecked(),
						    p._terminator->isChecked(),
						    p._rdelay.value,
						    p._rwidth.value,
						    0,
						    0, 
						    0);
  }  

  delete[] ec;

  return sizeof(EvrConfigType) + 
    nec*sizeof(EvrConfigType::EventCodeType) +
    npt*sizeof(EvrConfigType::PulseType) +
    nom*sizeof(EvrConfigType::OutputMapType);
}

QLayout* EvrPulseTable::initialize(QWidget* parent) 
{
  _qlink = new EvrPulseTableQ(*this,parent);

  //
  //  Read EvrIOConfig
  //
  /*
    if (_cfg.path) {
    for(unsigned i=0; i<MaxOutputs; i++)
    _outputs[i] = new QrLabel;
    }
    else
  */
  const EvrIOConfigType* iocfg = reinterpret_cast<const EvrIOConfigType*>(GlobalCfg::fetch(_evrIOConfigType));
  if (iocfg)
    for(unsigned i=0; i<iocfg->nchannels(); i++)
      _outputs[i] = new QrLabel(iocfg->channel(i).name());
  else
    for(unsigned i=0; i<MaxOutputs; i++)
      _outputs[i] = new QrLabel(QString::number(i));

  //  QString period = QString("%1%2%3%4 sec").arg(QChar(0x215F)).arg(QChar(0x2081)).arg(QChar(0x2082)).arg(QChar(0x2080));
  QString period("events");

  int row=0, column=0;
  Qt::Alignment align = Qt::AlignBottom | Qt::AlignHCenter;
  QGridLayout* layout = new QGridLayout;
  layout->addWidget(new QLabel("Enable\n")         , row, column++, align);
  layout->addWidget(new QLabel("Event\nCode\n")    , row, column++, align);
  layout->addWidget(new QLabel("Pulse\nPolarity\n"), row, column++, align);
  layout->addWidget(new QLabel("Pulse\nDelay\n(sec)")   , row, column++, align);
  layout->addWidget(new QLabel("Pulse\nWidth\n(sec)")   , row, column++, align);
  for(unsigned i=0; i<MaxOutputs; i++)
    layout->addWidget( _outputs[i], row, column++, align);
  layout->addWidget(new QLabel(QString("Report\nDelay\n(%1)").arg(period)) , row, column++, align);
  layout->addWidget(new QLabel(QString("Report\nWidth\n(%1)").arg(period)) , row, column++, align);
  layout->addWidget(new QLabel("Readout\n")        , row, column++, align);
  layout->addWidget(new QLabel("Terminator\n")     , row, column++, align);

  _enable_group     = new QButtonGroup(parent); _enable_group    ->setExclusive(false);
  _terminator_group = new QButtonGroup(parent); _terminator_group->setExclusive(false);
  _outputs_group    = new QButtonGroup(parent); _outputs_group   ->setExclusive(false);

  for(unsigned i=0; i<MaxPulses; i++)
    _pulses[i]->initialize(parent,layout,++row,i,_enable_group,_terminator_group,_outputs_group);

  ::QObject::connect(_enable_group    , SIGNAL(buttonClicked(int)), _qlink, SLOT(update_enable(int)));
  ::QObject::connect(_terminator_group, SIGNAL(buttonClicked(int)), _qlink, SLOT(update_terminator(int)));
  ::QObject::connect(_outputs_group   , SIGNAL(buttonClicked(int)), _qlink, SLOT(update_output(int)));
  if (Parameter::allowEdit())
    for(unsigned i=0; i<MaxPulses; i++)
      ::QObject::connect(_pulses[i]->_eventCode._input, SIGNAL(editingFinished()), _qlink, SLOT(update_eventcode()));

  return layout;
}

void EvrPulseTable::update() {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void EvrPulseTable::flush () {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void EvrPulseTable::enable(bool) 
{
}

void EvrPulseTable::update_enable    (int i) 
{
  if (i < 0) {
  }
  else if (i >= PolarityGroup) {
    _pulses[i-PolarityGroup]->enable(_enable_group->button(i-PolarityGroup)->isChecked());
  }
  else if (_enable_group->button(i)->isChecked()) {  // enabled
    _pulses[i]->enable(true);
  }
  else {  // disabled
    _pulses[i]->reset();
    _pulses[i]->enable(false);
  }
  flush();
}

void EvrPulseTable::update_terminator(int i) 
{
  //  Exclusive among enabled rows
  for(int j=0; j<MaxPulses; j++) {
    if (i==j || !_enable_group->button(j)->isChecked())
      continue;
    QAbstractButton* b = _pulses[j]->_terminator;
    if (b->isChecked())
      b->setChecked(false);
  }
}

void EvrPulseTable::update_output    (int k) 
{
  int row = k/MaxOutputs;
  int col = k%MaxOutputs;
  //  Exclusive among enabled rows
  for(int j=0; j<MaxPulses; j++) {
    if (row==j || !_enable_group->button(j)->isChecked())
      continue;
    QAbstractButton* b = _pulses[j]->_outputs[col];
    if (b->isChecked())
      b->setChecked(false);
  }
}

void EvrPulseTable::update_eventcode  ()
{
  for(unsigned i=MaxPulses-1; i>0; i--) {
    if (!_enable_group->button(i)->isChecked())
      continue;
    unsigned ec = _pulses[i]->_eventCode.value;
    bool shared=false;
    for(unsigned j=0; j<i; j++) {
      if (!_enable_group->button(j)->isChecked())
	continue;
      if (_pulses[j]->_eventCode.value == ec) {
	shared = true;
	break;
      }
    }
    _pulses[i]->_rdelay.widget()->setVisible(!shared);
    _pulses[i]->_rwidth.widget()->setVisible(!shared);
    _pulses[i]->_readout   ->setVisible(!shared);
    _pulses[i]->_terminator->setVisible(!shared);
  }
}

EvrPulseTableQ::EvrPulseTableQ(EvrPulseTable& table,QWidget* parent) : QObject(parent), _table(table) {}

void EvrPulseTableQ::update_enable    (int i) { _table.update_enable(i); }
void EvrPulseTableQ::update_terminator(int i) { _table.update_terminator(i); }
void EvrPulseTableQ::update_output    (int i) { _table.update_output(i); }
void EvrPulseTableQ::update_eventcode ()      { _table.update_eventcode(); }

#include "Parameters.icc"
