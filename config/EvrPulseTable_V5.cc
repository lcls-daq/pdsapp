#include "pdsapp/config/EvrPulseTable_V5.hh"

#include "pdsapp/config/QrLabel.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/PolarityButton.hh"
#include "pdsapp/config/EvrEventCodeTable.hh"
#include "pdsapp/config/EventcodeTiming.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/evr/IOConfigV1.hh"
#include "pdsdata/evr/IOChannel.hh"

static Pds::TypeId _evrIOConfigType(Pds::TypeId::Id_EvrIOConfig,
                                    Pds::EvrData::IOConfigV1::Version);


#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>

static const int PolarityGroup = 100;
static const double EvrPeriod = 1./119e6;

namespace Pds_ConfigDb
{
  class Pulse_V5 {
  public:
    Pulse_V5() :
      _delay    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvrPeriod)),
      _width    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvrPeriod))
    {}
  public:
    void enable(bool v) 
    {
      bool allowEdit = Parameter::allowEdit();
      _polarity  ->setEnabled(v && allowEdit);
      _polarity  ->setVisible(v);
      v &= (_polarity->state()!=PolarityButton::None);
      _delay    .enable(v);
      _width    .enable(v);
      for(unsigned i=0; i<Pds::EvrData::ConfigV5::EvrOutputs; i++) {
	_outputs[i]->setEnabled(v && allowEdit);
	_outputs[i]->setVisible(v);
      }
    }
    void reset () 
    {
      _delay    .value = 0;
      _width    .value = 0;
      _polarity  ->setState(PolarityButton::None);
      for(unsigned i=0; i<Pds::EvrData::ConfigV5::EvrOutputs; i++)
	_outputs[i]->setChecked(false);
    }
  public:
    void initialize(QWidget* parent,
		    QGridLayout* layout, 
		    int row, int bid,
		    QButtonGroup* egroup,
		    QButtonGroup* ogroup) 
    {
      _enable    = new QCheckBox;
      _polarity  = new PolarityButton;
      for(unsigned i=0; i<Pds::EvrData::ConfigV5::EvrOutputs; i++)
	_outputs[i] = new QCheckBox;

      egroup->addButton(_enable,bid);
      egroup->addButton(_polarity,bid+PolarityGroup);
      for(unsigned i=0; i<Pds::EvrData::ConfigV5::EvrOutputs; i++)
	ogroup->addButton(_outputs[i],bid*Pds::EvrData::ConfigV5::EvrOutputs+i);

      int column = 0;
      layout->addWidget(_enable, row, column++, Qt::AlignCenter);
      layout->addWidget(_polarity , row, column++, Qt::AlignCenter);
      layout->setColumnMinimumWidth(column,97);
      layout->addLayout(_delay.initialize(parent)    , row, column++, Qt::AlignCenter);
      layout->setColumnMinimumWidth(column,97);
      layout->addLayout(_width.initialize(parent)    , row, column++, Qt::AlignCenter);
      for(unsigned i=0; i<Pds::EvrData::ConfigV5::EvrOutputs; i++)
	layout->addWidget(_outputs[i], row, column++, Qt::AlignCenter);

      _delay    .widget()->setMaximumWidth(97);
      _width    .widget()->setMaximumWidth(97);

      ::QObject::connect(_polarity, SIGNAL(toggled(bool)), _delay.widget(), SLOT(setVisible(bool)));
      ::QObject::connect(_polarity, SIGNAL(toggled(bool)), _width.widget(), SLOT(setVisible(bool)));
      for(unsigned i=0; i<Pds::EvrData::ConfigV5::EvrOutputs; i++)
	::QObject::connect(_polarity, SIGNAL(toggled(bool)), _outputs[i], SLOT(setVisible(bool)));

      _enable->setEnabled(Parameter::allowEdit());

      reset();
      enable(false);
    }

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_delay);
      pList.insert(&_width);
    }

  public:
    QCheckBox*            _enable;
    PolarityButton*       _polarity;
    NumericInt<unsigned>  _delay;
    NumericInt<unsigned>  _width;
    QCheckBox*            _outputs[Pds::EvrData::ConfigV5::EvrOutputs];
  };
};

using namespace Pds_ConfigDb;

EvrPulseTable_V5::EvrPulseTable_V5(unsigned id) :
  Parameter(NULL),
  _id      (id),
  _npulses (0)
{
  for(unsigned i=0; i<MaxPulses; i++)
    _pulses[i] = new Pulse_V5;
}

EvrPulseTable_V5::~EvrPulseTable_V5()
{
}

void EvrPulseTable_V5::pull(const Pds::EvrData::ConfigV5& tc) {

  unsigned npulses     = 0;
  int      delay_offset= 0;
  for(unsigned i=0; i<tc.neventcodes(); i++) {
    const Pds::EvrData::ConfigV5::EventCodeType& ec = tc.eventcode(i);
    if (ec.isReadout()) {
      delay_offset =
        EventcodeTiming::timeslot(140) -
        EventcodeTiming::timeslot(ec.code());
      break;
    }
  }

  for(unsigned j=0; j<tc.npulses(); j++) {
    Pulse_V5& p = *_pulses[npulses];

    bool lUsed=false;
    for(unsigned k=0; k<Pds::EvrData::ConfigV5::EvrOutputs; k++)
      p._outputs[k]->setChecked(false);
    for(unsigned k=0; k<tc.noutputs(); k++) {
      const Pds::EvrData::ConfigV5::OutputMapType& om = tc.output_map(k);
      if ( om.source()==Pds::EvrData::ConfigV5::OutputMapType::Pulse &&
           om.source_id()==j )
        if ((om.conn_id()/Pds::EvrData::ConfigV5::EvrOutputs) == _id)
          p._outputs[om.conn_id()%Pds::EvrData::ConfigV5::EvrOutputs]->setChecked(lUsed=true);
      
    }
    if (!lUsed) continue;

    p._enable    ->setChecked(true);
    update_enable(npulses);
    const Pds::EvrData::ConfigV5::PulseType& pt = tc.pulse(j);
    p._polarity  ->setState(pt.polarity()==Pds_ConfigDb::Enums::Pos ? 
                            PolarityButton::Pos : PolarityButton::Neg);
    p._delay      .value = pt.delay() - delay_offset;
    p._width      .value = pt.width();
//     printf("delay/width %d/%d  :  %g/%g\n",
//            p._delay.value, p._width.value,
//            p._delay.scale, p._width.scale);
    npulses++;
  }
  while(npulses < MaxPulses) {
    _pulses[npulses]->_enable->setChecked(false);
    update_enable(npulses++);
  }
}

unsigned EvrPulseTable_V5::npulses() const {
  return _npulses;
}

unsigned EvrPulseTable_V5::noutputs() const {
  return _noutputs;
}


bool EvrPulseTable_V5::validate(unsigned ncodes, 
                             const Pds::EvrData::ConfigV5::EventCodeType* codes,
                             int delay_offset,
                             unsigned p0, Pds::EvrData::ConfigV5::PulseType* pt,
                             unsigned o0, Pds::EvrData::ConfigV5::OutputMapType* om)
{
  unsigned npt = 0;
  unsigned nom = 0;

  for(unsigned i=0; i<MaxPulses; i++) {
    const Pulse_V5& p = *_pulses[i];
    if (!p._enable->isChecked())
      continue;

    if (p._polarity->state() == PolarityButton::None)
      continue;

    int adjusted_delay = p._delay.value + delay_offset;

    if (adjusted_delay < 0) {
      QString msg = QString("Pulse %1 delay too small (by %2 ticks, %3 sec)\n")
        .arg(i)
        .arg(-adjusted_delay)
        .arg(double(-adjusted_delay)*EvrPeriod);
      QMessageBox::warning(0,"Input Error",msg);
      return false;
    }

    //
    // MRF EVR limited to 16-bit pulse width
    //
    /**
    if (npt>=4 && p._width.value>0xffff) {
      QString msg = QString("Pulse %1 width exceeds EVR limits (%3 sec) for pulse id >= 4\n")
        .arg(i)
        .arg(double(0xffff)*EvrPeriod);
      QMessageBox::warning(0,"Input Error",msg);
      return false;
    }
    **/

    *new(&pt[npt]) Pds::EvrData::ConfigV5::PulseType(npt+p0, 
                                            p._polarity->state() == PolarityButton::Pos ? 0 : 1,
                                            1,
                                            adjusted_delay,
                                            p._width.value);

    for(unsigned j=0; j<Pds::EvrData::ConfigV5::EvrOutputs; j++) {
      if (p._outputs[j]->isChecked())
        *new(&om[nom++]) Pds::EvrData::ConfigV5::OutputMapType( Pds::EvrData::ConfigV5::OutputMapType::Pulse, npt+p0,
                                                       Pds::EvrData::ConfigV5::OutputMapType::UnivIO, j+Pds::EvrData::ConfigV5::EvrOutputs*_id );
    }
    npt++;
  }

  uint32_t pm = ((1<<npt)-1)<<p0;
  uint32_t fill = 0;
  for(unsigned i=0; i<ncodes; i++)
    if (codes[i].isReadout())
      *new(const_cast<Pds::EvrData::ConfigV5::EventCodeType*>(&codes[i]))
           Pds::EvrData::ConfigV5::EventCodeType(codes[i].code(),
                                        codes[i].desc(),
                                        codes[i].maskTrigger()|pm,fill,fill);
           
  _npulses  = npt;
  _noutputs = nom;

  return true;
}



QLayout* EvrPulseTable_V5::initialize(QWidget*) 
{
  QVBoxLayout* vl = new QVBoxLayout;

  _qlink = new EvrPulseTable_V5Q(*this,0);

  //
  //  Read EvrIOConfig
  //
  unsigned j=0;
  { const char* p = reinterpret_cast<const char*>(GlobalCfg::fetch(_evrIOConfigType));
    if (p) {
      unsigned id=0;
      do {
        const Pds::EvrData::IOConfigV1& iocfg = *reinterpret_cast<const Pds::EvrData::IOConfigV1*>(p);
        if (iocfg.nchannels()==0) break;
        for(unsigned i=0; i<iocfg.nchannels(); i++)
          if (id == _id)
            _outputs[j++] = new QrLabel(iocfg.channel(i).name());
        p += iocfg.size();
        id++;
      } while(1);
    }
  }  
  while(j < Pds::EvrData::ConfigV5::EvrOutputs) {
    _outputs[j] = new QrLabel(QString::number(j));
    j++;
  }

  //  QString period = QString("%1%2%3%4 sec").arg(QChar(0x215F)).arg(QChar(0x2081)).arg(QChar(0x2082)).arg(QChar(0x2080));
  QString period("events");

  int row=0, column=0;
  Qt::Alignment align = Qt::AlignBottom | Qt::AlignHCenter;
  QGridLayout* layout = new QGridLayout;
  layout->addWidget(new QLabel("Enable\n")         , row, column++, align);
  layout->addWidget(new QLabel("Pulse\nPolarity\n"), row, column++, align);
  layout->addWidget(new QLabel("Pulse\nDelay\n(sec)")   , row, column++, align);
  layout->addWidget(new QLabel("Pulse\nWidth\n(sec)")   , row, column++, align);
  for(unsigned i=0; i<Pds::EvrData::ConfigV5::EvrOutputs; i++)
    layout->addWidget( _outputs[i], row, column++, align);

  _enable_group     = new QButtonGroup; _enable_group    ->setExclusive(false);
  _outputs_group    = new QButtonGroup; _outputs_group   ->setExclusive(false);

  for(unsigned i=0; i<MaxPulses; i++) {
    _pulses[i]->initialize(0,layout,++row,i,_enable_group,_outputs_group);
    _pulses[i]->insert(_pList);
  }

  ::QObject::connect(_enable_group    , SIGNAL(buttonClicked(int)), _qlink, SLOT(update_enable(int)));
  ::QObject::connect(_outputs_group   , SIGNAL(buttonClicked(int)), _qlink, SLOT(update_output(int)));

  vl->addStretch();
  { QGridLayout* hl = new QGridLayout;
    hl->addWidget(new QLabel("Pulses generated by \"Readout\" EventCode")             ,0,0,Qt::AlignHCenter);
    hl->addWidget(new QLabel("Pulse delay is specified with respect to EventCode 140"),1,0,Qt::AlignHCenter);
    vl->addLayout(hl); }
  vl->addStretch();
  vl->addLayout(layout);
  vl->addStretch();

  return vl;
}

void EvrPulseTable_V5::update() {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void EvrPulseTable_V5::flush () {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void EvrPulseTable_V5::enable(bool) 
{
}

void EvrPulseTable_V5::update_enable    (int i) 
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

void EvrPulseTable_V5::update_output    (int k) 
{
  int row = k/Pds::EvrData::ConfigV5::EvrOutputs;
  int col = k%Pds::EvrData::ConfigV5::EvrOutputs;
  //  Exclusive among enabled rows
  for(int j=0; j<MaxPulses; j++) {
    if (row==j || !_enable_group->button(j)->isChecked())
      continue;
    QAbstractButton* b = _pulses[j]->_outputs[col];
    if (b->isChecked())
      b->setChecked(false);
  }
}

EvrPulseTable_V5Q::EvrPulseTable_V5Q(EvrPulseTable_V5& table,QWidget* parent) : QObject(parent), _table(table) {}

void EvrPulseTable_V5Q::update_enable    (int i) { _table.update_enable(i); }
void EvrPulseTable_V5Q::update_output    (int i) { _table.update_output(i); }

#include "Parameters.icc"
