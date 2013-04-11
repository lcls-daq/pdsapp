#include "pdsapp/config/EvrPulseTable.hh"

#include "pdsapp/config/QrLabel.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/PolarityButton.hh"
#include "pdsapp/config/EvrEventCodeTable.hh"
#include "pdsapp/config/EventcodeTiming.hh"
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
static const double EvrPeriod = 1./119e6;

namespace Pds_ConfigDb
{
  class Pulse {
  public:
    Pulse() :
      _delay    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvrPeriod)),
      _width    ( NumericInt<unsigned>(NULL,0,0, 0x7fffffff, Scaled, EvrPeriod))
    {}
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
      _group  ->setVisible(v && enableGroup);
      if (!enableGroup)
        _group->setCurrentIndex(0);
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
      
      _group = new QComboBox;
      for (int iGroup = 1; iGroup <= EvrConfigType::EventCodeType::MaxReadoutGroup; ++iGroup)
        _group->addItem(QString().setNum(iGroup));
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
    QComboBox*            _group;
    NumericInt<unsigned>  _delay;
    NumericInt<unsigned>  _width;
    QCheckBox*            _outputs[MaxOutputs];
  };
};

using namespace Pds_ConfigDb;

EvrPulseTable::EvrPulseTable(unsigned id) :
  Parameter(NULL),
  _id      (id),
  _npulses (0),
  _bEnableReadGroup(false),
  _pLabelGroup(NULL)
{
  for(unsigned i=0; i<MaxPulses; i++)
    _pulses[i] = new Pulse;
}

EvrPulseTable::~EvrPulseTable()
{
}

void EvrPulseTable::pull(const EvrConfigType& tc) {  
  unsigned npulses     = 0;
  for(unsigned j=0; j<tc.npulses(); j++) {
    Pulse& p = *_pulses[npulses];

    bool lUsed=false;
    for(unsigned k=0; k<MaxOutputs; k++)
      p._outputs[k]->setChecked(false);
    for(unsigned k=0; k<tc.noutputs(); k++) {
      const EvrConfigType::OutputMapType& om = tc.output_map(k);
      if ( om.source()==EvrConfigType::OutputMapType::Pulse &&
           om.source_id()==j )
        if ((om.module()) == _id)
          p._outputs[om.conn_id()]->setChecked(lUsed=true);      
    }
    if (!lUsed) continue;

    p._enable    ->setChecked(true);
    update_enable(npulses);
    const EvrConfigType::PulseType& pt = tc.pulse(j);
        
    const uint32_t  uPulseBit    = (1 << j);
    const EvrConfigType::EventCodeType* 
                    pCodeReadout = NULL;
    for(unsigned i=0; i<tc.neventcodes(); i++) {
      const EvrConfigType::EventCodeType& ec = tc.eventcode(i);
      if (ec.isReadout() && ( ec.maskTrigger() & uPulseBit ) != 0) {
        //printf( "EvrPulseTable::pull(): pulse %d event code [%d] %d readout group %d maskTrigger 0x%x\n", 
        //  j, i, ec.code(), ec.readoutGroup(), ec.maskTrigger()); //!!!debug

        if ( pCodeReadout == NULL )
          pCodeReadout = &ec;          
        // !!! don't warn about secondary readout eventcode
        //else
        //{
        //  QString msg = QString("Pulse %1: Secondary readout eventcode [%2] %3 found. Conflict with primary readout %4\n")
        //    .arg(j)
        //    .arg(i)
        //    .arg(ec.code())
        //    .arg(pCodeReadout->code());
        //  QMessageBox::warning(0,"",msg);                  
        //}                
      }
    }

    int iDelayOffset;    
    if ( pCodeReadout != NULL) 
    {
      iDelayOffset = 
        EventcodeTiming::timeslot(140) -
        EventcodeTiming::timeslot(pCodeReadout->code());        
      p._group->setCurrentIndex(pCodeReadout->readoutGroup()-1);    
    }
    else
    {
      iDelayOffset = 0;
      p._group->setCurrentIndex(0);    
    }
    p._polarity  ->setState(pt.polarity()==Pds_ConfigDb::Enums::Pos ? 
                            PolarityButton::Pos : PolarityButton::Neg);
    p._delay      .value = pt.delay() - iDelayOffset;
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
  
  printf("EvrPulseTable pull %d pulses %d eventcodes\n", npulses, tc.neventcodes());
}

unsigned EvrPulseTable::npulses() const {
  return _npulses;
}

unsigned EvrPulseTable::noutputs() const {
  return _noutputs;
}


bool EvrPulseTable::validate(unsigned ncodes, 
                             const EvrConfigType::EventCodeType* codes,
                             //int delay_offset,
                             unsigned p0, EvrConfigType::PulseType* pt,
                             unsigned o0, EvrConfigType::OutputMapType* om
                             )
{  
  unsigned npt = 0;
  unsigned nom = 0;

  for(unsigned i=0; i<MaxPulses; i++) {
    const Pulse& p = *_pulses[i];
    if (!p._enable->isChecked())
      continue;

    if (p._polarity->state() == PolarityButton::None)
      continue;

    for(unsigned j=0; j<MaxOutputs; j++) {
      if (p._outputs[j]->isChecked())
        *new(&om[nom++]) EvrConfigType::OutputMapType( EvrConfigType::OutputMapType::Pulse, npt+p0,
                                                       EvrConfigType::OutputMapType::UnivIO, j, _id );
    }    
    
    int delay_offset = 0;
    int primary_readout = -1;
    
    int iGroup = 1+p._group->currentIndex();
    uint32_t pm = 1<<(npt+p0);
    uint32_t fill = 0;
    for(unsigned i=0; i<ncodes; i++)
      if (codes[i].isReadout() && codes[i].readoutGroup() == iGroup )
      {
        *new(const_cast<EvrConfigType::EventCodeType*>(&codes[i]))
             EvrConfigType::EventCodeType(codes[i].code(),
                                          codes[i].readoutGroup(),
                                          codes[i].desc(),
                                          codes[i].maskTrigger()|pm,fill,fill);
                                          
        if ( primary_readout == -1 )
        {
          primary_readout = codes[i].code();
          delay_offset    =
                    EventcodeTiming::timeslot(140) -
                    EventcodeTiming::timeslot(codes[i].code());                                                                                    
          printf("Adjusting pulse [%d] for readout event code [%d] %d, delays %+d ticks (%lg ns)\n",
                 npt, i, codes[i].code(), delay_offset, (double)(delay_offset*EvrPeriod*1e9));
        }
        //!!! dont warn about the secondary readout eventcode
        //else
        //{
        //  int delay = delay_offset -
        //    (EventcodeTiming::timeslot(140) -
        //     EventcodeTiming::timeslot(codes[i].code()));
        //  QString msg = QString("Secondary readout eventcode [%1] %2 will produce pulses %3 ticks (%4 ns)\n")
        //    .arg(i)
        //    .arg(codes[i].code())
        //    .arg(delay)
        //    .arg(delay*EvrPeriod*1e9);
        //  msg += QString("delayed with respect to primary readout eventcode %1\n").arg(primary_readout);
        //  QMessageBox::warning(0,"",msg);          
        //}
      }
                                        
    int adjusted_delay = p._delay.value + delay_offset;

    if (adjusted_delay < 0) {
      QString msg = QString("Pulse %1 delay too small (by %2 ticks, %3 ns)\n")
        .arg(npt)
        .arg(-adjusted_delay)
        .arg(double(-adjusted_delay)*EvrPeriod*1e9);
      QMessageBox::warning(0,"Input Error",msg);
      return false;
    }
                                        
    *new(&pt[npt]) EvrConfigType::PulseType(npt+p0, 
                                            p._polarity->state() == PolarityButton::Pos ? 0 : 1,
                                            1,
                                            adjusted_delay,
                                            p._width.value);
    
    npt++;
  }
           
  _npulses  = npt;
  _noutputs = nom;

  return true;
}



QLayout* EvrPulseTable::initialize(QWidget*) 
{
  QVBoxLayout* vl = new QVBoxLayout;

  _qlink = new EvrPulseTableQ(*this,0);

  //
  //  Read EvrIOConfig
  //
  unsigned j=0;
  { const char* p = reinterpret_cast<const char*>(GlobalCfg::fetch(_evrIOConfigType));
    if (p) {
      unsigned id=0;
      do {
        const EvrIOConfigType& iocfg = *reinterpret_cast<const EvrIOConfigType*>(p);
        if (iocfg.nchannels()==0) break;
        for(unsigned i=0; i<iocfg.nchannels(); i++)
          if (id == _id)
            _outputs[j++] = new QrLabel(iocfg.channel(i).name());
        p += iocfg.size();
        id++;
      } while(1);
    }
  }  
  while(j < MaxOutputs) {
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
  { QGridLayout* hl = new QGridLayout;
    hl->addWidget(new QLabel("Pulses generated by \"Readout\" EventCode")             ,0,0,Qt::AlignHCenter);
    hl->addWidget(new QLabel("Pulse delay is specified with respect to EventCode 140"),1,0,Qt::AlignHCenter);
    vl->addLayout(hl); }
  vl->addStretch();
  vl->addLayout(layout);
  vl->addStretch();

  return vl;
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

void EvrPulseTable::enable(bool) // virtual function. Need to be defined
{
}

void EvrPulseTable::setReadGroupEnable(bool bEnableReadGroup)
{
  _bEnableReadGroup = bEnableReadGroup;
  if (_pLabelGroup != NULL)
    _pLabelGroup->setVisible(_bEnableReadGroup);
  
  for (int iPulse = 0; iPulse < MaxPulses; ++iPulse)
    update_enable(iPulse);
}

void EvrPulseTable::update_enable    (int i) 
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

EvrPulseTableQ::EvrPulseTableQ(EvrPulseTable& table,QWidget* parent) : QObject(parent), _table(table) {}

void EvrPulseTableQ::update_enable    (int i) { _table.update_enable(i); }
void EvrPulseTableQ::update_output    (int i) { _table.update_output(i); }

EvrPulseTables::EvrPulseTables() : 
  Parameter(NULL), 
  _nevr(0),
  _pulse_buffer (new char[EvrConfigType::MaxPulses*sizeof(EvrConfigType::PulseType)]),
  _output_buffer(new char[EvrConfigType::MaxOutputs*sizeof(EvrConfigType::OutputMapType)]),
  _npulses      (0),
  _noutputs     (0)
{
  for(unsigned i=0; i<MaxEVRs; i++)
    _evr[i] = new EvrPulseTable(i);
}

EvrPulseTables::~EvrPulseTables() {
  delete[] _pulse_buffer;
  delete[] _output_buffer;
  for(unsigned i=0; i<MaxEVRs; i++)
    delete _evr[i];
}

QLayout* EvrPulseTables::initialize(QWidget*) 
{
  QVBoxLayout* vl = new QVBoxLayout;
  vl->addStretch();
  { QGridLayout* hl = new QGridLayout;
    hl->addWidget(new QLabel("Pulses generated by \"Readout\" EventCode")             ,0,0,Qt::AlignHCenter);
    hl->addWidget(new QLabel("Pulse delay is specified with respect to EventCode 140"),1,0,Qt::AlignHCenter);
    vl->addLayout(hl); }
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

  _nevr = 1;
  _tab->setTabEnabled(0,true);

  return vl;
}

void EvrPulseTables::setReadGroupEnable(bool bEnableReadGroup)
{
  for(unsigned i=0; i<MaxEVRs; i++)
    _evr[i]->setReadGroupEnable(bEnableReadGroup);
}

void EvrPulseTables::pull    (const EvrConfigType& tc)
{ 
  for(unsigned i=0; i<MaxEVRs; i++) 
    _evr[i]->pull(tc); 

  //
  //  Read EvrIOConfig
  //
  unsigned nevr = 1;
  { const char* p = reinterpret_cast<const char*>(GlobalCfg::fetch(_evrIOConfigType));
    if (p) {
      nevr = 0;
      do {
        const EvrIOConfigType& iocfg = *reinterpret_cast<const EvrIOConfigType*>(p);
        if (iocfg.nchannels()==0) break;
        p += iocfg.size();
        nevr++;
      } while(1);
    }
  }
  
  for(unsigned i=nevr; i<_nevr; i++)
    _tab->setTabEnabled(i,false);
  for(unsigned i=_nevr; i<nevr; i++)
    _tab->setTabEnabled(i,true);

  _nevr = nevr;
}

bool EvrPulseTables::validate(unsigned ncodes,
                  const EvrConfigType::EventCodeType* codes)
{
  bool result  = true;

  unsigned npt = 0;
  EvrConfigType::PulseType*     pt = 
    reinterpret_cast<EvrConfigType::PulseType*>(_pulse_buffer);

  unsigned nom = 0;
  EvrConfigType::OutputMapType* om = 
    reinterpret_cast<EvrConfigType::OutputMapType*>(_output_buffer);
  
  for(unsigned i=0; i<_nevr; i++) {
    //printf("Pds_ConfigDb::EvrPulseTables::validate() evr %d\n", i);//!!!debug
    result &= _evr[i]->validate(ncodes, codes, 
                                //delay_offset, 
                                npt, pt,
                                nom, om
                                );
    npt += _evr[i]->npulses();
    //printf("Pds_ConfigDb::EvrPulseTables:: evr %d pulse %d \n", i, _evr[i]->npulses());//!!!debug
    pt  += _evr[i]->npulses();
    nom += _evr[i]->noutputs();
    om  += _evr[i]->noutputs();
  }
  _npulses  = npt;
  _noutputs = nom;

  //printf("Pds_ConfigDb::EvrPulseTables:: pulses %d\n", _npulses);//!!!debug
  return result;
}


#include "Parameters.icc"
