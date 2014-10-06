#include "pdsapp/config/EvsPulseTable.hh"

#include "pdsapp/config/QrLabel.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/PolarityButton.hh"
#include "pdsapp/config/EvrEventCodeTable.hh"
#include "pds/config/EvrIOConfigType.hh"

#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QComboBox>

#include <stdio.h>

static const unsigned MaxOutputs = 13;
static const int PolarityGroup = 100;
static const double EvsPeriod = 1./119e6;

namespace Pds_ConfigDb
{
  class EvsPulse {
  public:
    EvsPulse() :
      _delay    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvsPeriod)),
      _width    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvsPeriod)),
      _group    ( new QComboBox )
    {
      _group->addItem("Persistent");
      _group->addItem("Running");
    }
  public:
    void enable(bool v, bool enableGroup) 
    {
      bool allowEdit = _delay.allowEdit();
      _polarity  ->setEnabled(v && allowEdit);
      _polarity  ->setVisible(v);
      v &= (_polarity->state()!=PolarityButton::None);
      _delay    .enable(v);
      _width    .enable(v);
      _group  ->setEnabled(v && allowEdit);
      for(unsigned i=0; i<MaxOutputs; i++) {
        _outputs[i]->setEnabled(v && allowEdit);
        _outputs[i]->setVisible(v);
      }
    }
    void reset () 
    {
      _delay    .value = 0;
      _width    .value = 0;      
      _polarity  ->setState(PolarityButton::None);
      _group     ->setCurrentIndex(0);
      for(unsigned i=0; i<MaxOutputs; i++)
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
      for(unsigned i=0; i<MaxOutputs; i++)
        _outputs[i] = new QCheckBox;

      egroup->addButton(_enable,bid);
      egroup->addButton(_polarity,bid+PolarityGroup);
      for(unsigned i=0; i<MaxOutputs; i++)
        ogroup->addButton(_outputs[i],bid*MaxOutputs+i);

      int column = 0;
      layout->addWidget(_enable, row, column++, Qt::AlignCenter);
      layout->addWidget(_polarity , row, column++, Qt::AlignCenter);
      layout->setColumnMinimumWidth(column,97);
      layout->addLayout(_delay.initialize(parent)    , row, column++, Qt::AlignCenter);
      layout->setColumnMinimumWidth(column,97);
      layout->addLayout(_width.initialize(parent)    , row, column++, Qt::AlignCenter);
      
      _group->setCurrentIndex(0);
      layout->addWidget(_group, row, column++, Qt::AlignCenter);
      
      for(unsigned i=0; i<MaxOutputs; i++)
        layout->addWidget(_outputs[i], row, column++, Qt::AlignCenter);

      _delay    .widget()->setMaximumWidth(97);
      _width    .widget()->setMaximumWidth(97);

      ::QObject::connect(_polarity, SIGNAL(toggled(bool)), _delay.widget(), SLOT(setVisible(bool)));
      ::QObject::connect(_polarity, SIGNAL(toggled(bool)), _width.widget(), SLOT(setVisible(bool)));
      for(unsigned i=0; i<MaxOutputs; i++)
        ::QObject::connect(_polarity, SIGNAL(toggled(bool)), _outputs[i], SLOT(setVisible(bool)));

      _enable->setEnabled(_delay.allowEdit());

      reset();
      enable(false, false);
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
    QComboBox*            _group;
    QCheckBox*            _outputs[MaxOutputs];
  };
};

using namespace Pds_ConfigDb;

EvsPulseTable::EvsPulseTable(unsigned id) :
  Parameter(NULL),
  _id           (id),
  _pulse_buffer (new char[EvrConfigType::MaxPulses *sizeof(PulseType)]),
  _output_buffer(new char[EvrConfigType::MaxOutputs*sizeof(OutputMapType)]),
  _npulses      (0),
  _noutputs     (0)
{
  for(unsigned i=0; i<MaxPulses; i++)
    _pulses[i] = new EvsPulse;
}

EvsPulseTable::~EvsPulseTable()
{
  delete _qlink;
  delete[] _pulse_buffer;
  delete[] _output_buffer;
}

bool EvsPulseTable::pull(const EvsConfigType& tc)
{
  unsigned npulses     = 0;
  for(unsigned j=0; j<tc.npulses(); j++) {
    EvsPulse& p = *_pulses[npulses];

    bool lUsed=false;
    for(unsigned k=0; k<MaxOutputs; k++)
      p._outputs[k]->setChecked(false);
    for(unsigned k=0; k<tc.noutputs(); k++) {
      const OutputMapType& om = tc.output_maps()[k];
      if ( om.source()==OutputMapType::Pulse &&
           om.source_id()==j )
        if ((om.module()) == _id)
	  p._outputs[om.conn_id()]->setChecked(lUsed=true);      
    }
    if (!lUsed) continue;

    p._enable    ->setChecked(true);
    update_enable(npulses);
    const PulseType& pt = tc.pulses()[j];
        
    unsigned readoutGroup = 0;
    const uint32_t  uPulseBit    = (1 << j);
    for(unsigned i=0; i<tc.neventcodes(); i++) {
      const EvsCodeType& ec = tc.eventcodes()[i];
      if ((ec.maskTriggerP() & uPulseBit ) != 0) {
	readoutGroup = 0;
	break;
      }
      if ((ec.maskTriggerR() & uPulseBit ) != 0) {
	readoutGroup = 1;
	break;
      }
    }

    p._group->setCurrentIndex(readoutGroup);    
    p._polarity  ->setState(pt.polarity()==Pds_ConfigDb::Enums::Pos ? 
                            PolarityButton::Pos : PolarityButton::Neg);
    p._delay      .value = pt.delay();
    p._width      .value = pt.width();
    npulses++;
  }

  bool result = npulses>0;

  while(npulses < MaxPulses) {
    _pulses[npulses]->_enable->setChecked(false);
    update_enable(npulses++);
  }  
  
  return result;
}

bool EvsPulseTable::validate(unsigned ncodes, 
                             const EvsCodeType* codes,
			     unsigned p0, PulseType* pt,
                             unsigned o0, OutputMapType* om)
{
  bool result = true;

  unsigned npt = 0;
  unsigned nom = 0;

  for(unsigned i=0; i<MaxPulses; i++) {
    const EvsPulse& p = *_pulses[i];
    if (!p._enable->isChecked())
      continue;

    if (p._polarity->state() == PolarityButton::None)
      continue;

    for(unsigned j=0; j<MaxOutputs; j++) {
      if (p._outputs[j]->isChecked())
        *new(&om[nom++]) OutputMapType( OutputMapType::Pulse, npt+p0,
                                        OutputMapType::UnivIO, j, _id );
    }    
    
    uint32_t pm=0, rm=0;
    if (p._group->currentIndex())
      rm = 1<<(npt+p0);
    else
      pm = 1<<(npt+p0);

    for(unsigned i=0; i<ncodes; i++)
      *new(const_cast<EvsCodeType*>(&codes[i]))
	EvsCodeType(codes[i].code(),
		    codes[i].period(),
		    codes[i].maskTriggerP()|pm,
		    codes[i].maskTriggerR()|rm,
		    codes[i].desc(),
		    codes[i].readoutGroup());
                                          
                                        
    *new(&pt[npt]) PulseType(npt+p0, 
                             p._polarity->state() == PolarityButton::Pos ? 0 : 1,
                             1,
                             p._delay.value,
                             p._width.value);
    
    npt++;
  }
           
  _npulses  = npt;
  _noutputs = nom;

  return result;
}

QLayout* EvsPulseTable::initialize(QWidget*) 
{
  QVBoxLayout* vl = new QVBoxLayout;

  _qlink = new EvsPulseTableQ(*this,0);

  for(unsigned i=0; i<MaxOutputs; i++)
    _outputs[i] = new QrLabel(QString::number(i));

  //
  //  Read EvrIOConfig
  //
  { 
    const char* p = reinterpret_cast<const char*>(GlobalCfg::instance().fetch(_evrIOConfigType));
    if (p) {
      const EvrIOConfigType& iocfg = *reinterpret_cast<const EvrIOConfigType*>(p);
      for(unsigned i=0; i<iocfg.nchannels(); i++) {
	const EvrIOChannelType& ch = iocfg.channels()[i];
	if (ch.output().module()==_id &&
	    strlen(iocfg.channels()[i].name()))
	  _outputs[ch.output().conn_id()] = new QrLabel(ch.name());
      }
    }
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
  layout->addWidget(_pLabelGroup = new QLabel("Group")  , row, column++, align);
  for(unsigned i=0; i<MaxOutputs; i++)
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
  vl->addLayout(layout);
  vl->addStretch();

  return vl;
}

void EvsPulseTable::update() {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void EvsPulseTable::flush () {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void EvsPulseTable::enable(bool) // virtual function. Need to be defined
{
}

void EvsPulseTable::update_enable    (int i) 
{
  if (i < 0) {
  }
  else if (i >= PolarityGroup) {
    _pulses[i-PolarityGroup]->enable(_enable_group->button(i-PolarityGroup)->isChecked(), _bEnableReadGroup);
  }
  else if (_enable_group->button(i)->isChecked()) {  // enabled
    _pulses[i]->enable(true, _bEnableReadGroup);
  }
  else {  // disabled
    _pulses[i]->reset();
    _pulses[i]->enable(false, _bEnableReadGroup);
  }
  flush();
}

void EvsPulseTable::update_output    (int k) 
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

EvsPulseTableQ::EvsPulseTableQ(EvsPulseTable& table,QWidget* parent) : QObject(parent), _table(table) {}

void EvsPulseTableQ::update_enable    (int i) { _table.update_enable(i); }
void EvsPulseTableQ::update_output    (int i) { _table.update_output(i); }

EvsPulseTables::EvsPulseTables() : 
  Parameter(NULL), 
  _pulse_buffer (new char[EvsConfigType::MaxPulses *sizeof(PulseType)]),
  _output_buffer(new char[EvsConfigType::MaxOutputs*sizeof(OutputMapType)]),
  _npulses      (0),
  _noutputs     (0)
{
  for(unsigned i=0; i<MaxEVRs; i++)
    _evr[i] = new EvsPulseTable(i);
}

EvsPulseTables::~EvsPulseTables() {
  delete[] _pulse_buffer;
  delete[] _output_buffer;
  for(unsigned i=0; i<MaxEVRs; i++)
    delete _evr[i];
}

QLayout* EvsPulseTables::initialize(QWidget*) 
{
  QVBoxLayout* vl = new QVBoxLayout;
  vl->addStretch();
  { _tab = new QTabWidget;
    for(unsigned i=0; i<MaxEVRs; i++) {
      QWidget* w = new QWidget;
      w->setLayout(_evr[i]->initialize(0));
      _tab->addTab(w,QString("EVR %1").arg(i));
      _tab->setTabEnabled(i,false);
    }
    vl->addWidget(_tab); }
  vl->addStretch();

  _tab->setTabEnabled(0,true);

  return vl;
}

void EvsPulseTables::pull    (const EvsConfigType& tc)
{ 
  unsigned mask=0;
  for(unsigned i=0; i<MaxEVRs; i++) 
    if (_evr[i]->pull(tc))
      mask |= (1<<i);

  //
  //  Read EvrIOConfig
  //
  { 
    const char* p = reinterpret_cast<const char*>(GlobalCfg::instance().fetch(_evrIOConfigType));
    if (p) {
      const EvrIOConfigType& iocfg = *reinterpret_cast<const EvrIOConfigType*>(p);
      for(unsigned i=0; i<iocfg.nchannels(); i++) {
	const EvrIOChannelType& ch = iocfg.channels()[i];
	mask |= (1<<ch.output().module());
      }
    }
  }  

  for(unsigned i=0; i<MaxEVRs; i++)
    _tab->setTabEnabled(i,mask&(1<<i));
}

bool EvsPulseTables::validate(unsigned ncodes,
                              const EvsCodeType* codes)
{
  bool result  = true;

  unsigned npt = 0;
  PulseType*     pt = 
    reinterpret_cast<PulseType*>(_pulse_buffer);

  unsigned nom = 0;
  OutputMapType* om = 
    reinterpret_cast<OutputMapType*>(_output_buffer);
  
  for(unsigned i=0; i<MaxEVRs; i++) {
    if (!_tab->isTabEnabled(i)) continue;

    //printf("Pds_ConfigDb::EvsPulseTables::validate() evr %d\n", i);//!!!debug
    result &= _evr[i]->validate(ncodes, codes, 
                                //delay_offset, 
                                npt, pt,
                                nom, om
                                );
    npt += _evr[i]->npulses();
    //printf("Pds_ConfigDb::EvsPulseTables:: evr %d pulse %d \n", i, _evr[i]->npulses());//!!!debug
    pt  += _evr[i]->npulses();
    nom += _evr[i]->noutputs();
    om  += _evr[i]->noutputs();
  }
  _npulses  = npt;
  _noutputs = nom;

  //printf("Pds_ConfigDb::EvsPulseTables:: pulses %d\n", _npulses);//!!!debug
  return result;
}


#include "Parameters.icc"
