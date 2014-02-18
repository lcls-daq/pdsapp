#include "pdsapp/config/EvrEventCodeTable.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/EvrSeqEventDesc.hh"
#include "pdsapp/config/EvrGlbEventDesc.hh"
#include "pdsapp/config/EvrEventDefault.hh"

//#include "pds/config/SeqConfigType.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>
#include <QtGui/QScrollArea>

#include <stdio.h>

static const unsigned MaxUserCodes      = Pds_ConfigDb::EvrEventCodeTable::MaxCodes;
static const unsigned MaxGlobalCodes    = 4;

static void showLayoutItem(QLayoutItem* item, bool show);

using namespace Pds_ConfigDb;


EvrEventCodeTable::EvrEventCodeTable(EvrPulseTables* pPulseTables) : 
  Parameter(NULL),
  _pPulseTables(pPulseTables),
  _ncodes     (0),
  _pLabelGroup1     (NULL),
  _pLabelGroup2     (NULL),
  _cbEnableReadGroup(NULL),
  _code_buffer(new char[(MaxUserCodes+MaxGlobalCodes)
                        *sizeof(EventCodeType)])                        
{
  _seq_code = new EvrSeqEventDesc[MaxUserCodes];
  _glb_code = new EvrGlbEventDesc[MaxGlobalCodes];
  _defaults = Parameter::readFromData() ? new EvrEventDefault : 0;
}

EvrEventCodeTable::~EvrEventCodeTable()
{
  delete[] _seq_code;
  delete[] _glb_code;
  delete[] _code_buffer;
}

void EvrEventCodeTable::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);
}

void EvrEventCodeTable::pull(const EvrConfigType& cfg) 
{  
  for(unsigned i=0; i<MaxUserCodes; i++)
    _seq_code[i].set_enable(false);

  for(unsigned i=0; i<MaxGlobalCodes; i++)
    _glb_code[i].set_enable(false);

  if (_defaults)
    _defaults->clear();

  unsigned nglb=0;
  unsigned nseq=0;

  bool bEnableReadoutGroup = false;
  for(unsigned i=0; i<cfg.neventcodes(); i++) {
    const EventCodeType& e = cfg.eventcodes()[i];
    if (e.readoutGroup() > 1)
      bEnableReadoutGroup = true;
    if (_defaults && _defaults->pull(e))
      continue;
    if (EvrGlbEventDesc::global_code(e.code())) {
      if (nglb < MaxGlobalCodes)
        _glb_code[nglb++].pull(e);
      continue;
    }
    if (nseq < MaxUserCodes)
      _seq_code[nseq++].pull(e);
  }
  
  _cbEnableReadGroup->setCurrentIndex(bEnableReadoutGroup? 1 : 0);      
}

//
//  Valid criteria:
//    Every "latch" type has a matching "release"
//    No code is assigned two types
//    Readout groups only contain one "readout" type or one "trigger" type
//
bool EvrEventCodeTable::validate() {

  bool result=true;

  EventCodeType* codep = 
    reinterpret_cast<EventCodeType*>(_code_buffer);

  //  Every "latch" type should have a partner un-"latch"
  unsigned latch  [MaxUserCodes];
  unsigned release[MaxUserCodes];
  unsigned latchmask=0;

  for(unsigned i=0; i<MaxUserCodes; i++) {
    _seq_code[i].push(codep);
    if (!_seq_code[i].enabled())
      continue;
    if (codep->code()==EvrEventDesc::Disabled)
      continue;

    if (codep->isLatch()) {
      latchmask |= (1<<i);
      latch  [i] = codep->code();
      release[i] = codep->releaseCode();
    }
  }

  for(unsigned i=0; i<MaxUserCodes; i++)
    if (latchmask & (1<<i)) {
      bool matched=false;
      for(unsigned j=0; j<MaxUserCodes; j++)
        if ((latchmask & (1<<j)) &&
            latch  [i]==release[j] &&
            release[i]==latch  [j]) {
          matched=true;
          break;
        }
      if (!matched) {
        QString msg = QString("LATCH event code %1 has no matching latch release code.\n").arg(latch[i]);
        msg += QString("Release code (%1) must be of LATCH type with complementing release code").arg(release[i]);
        QMessageBox::warning(0,"Input Error",msg);
        result=false;
      }
    }

  for(unsigned i=0; i<MaxUserCodes; i++)
    if (_seq_code[i].enabled())
      _seq_code[i].push(codep++);

  for(unsigned i=0; i<MaxGlobalCodes; i++)
    if (_glb_code[i].enabled())
      _glb_code[i].push(codep++);

  if (_defaults)
    _defaults->push(codep);

  _ncodes = codep - reinterpret_cast<EventCodeType*>(_code_buffer);

  { const EventCodeType* pcode = reinterpret_cast<const EventCodeType*>(_code_buffer);
    std::list<unsigned> dcodes;
    for(int i=0; i<int(_ncodes)-1; i++)
      for(int j=i+1; j<int(_ncodes); j++)
        if (pcode[j].code() == pcode[i].code())
          dcodes.push_back(pcode[i].code());
    if (dcodes.size()) {
      dcodes.sort();
      dcodes.unique();
      std::list<unsigned>::iterator it=dcodes.begin();
      QString msg = QString("Event codes specified multiple times {%1").arg(*it);
      while(++it != dcodes.end())
        msg += QString(",%1").arg(*it);
      msg += QString("}");
      QMessageBox::warning(0,"Input Error",msg);
      result=false;
    }

    std::vector<unsigned> tcodes(EventCodeType::MaxReadoutGroup+1);
    for(int i=0; i<int(_ncodes); i++)
      if (pcode[i].maskTrigger())
        tcodes[pcode[i].readoutGroup()]++;

    bool tcodes_fail=false;
    QString msg;
    for(int i=0; i<EventCodeType::MaxReadoutGroup+1; i++)
      if (tcodes[i]>1) {
        if (!tcodes_fail) {
          tcodes_fail=true;
          msg += QString("Multiple Readout/Trigger codes for groups {%1").arg(i);
        }
        else
          msg += QString(",%1").arg(i);
      }
    if (tcodes_fail) {
      msg += QString("}");
      QMessageBox::warning(0,"Input Error",msg);
      result=false;
    }
  }
    
  return result;
}

QLayout* EvrEventCodeTable::initialize(QWidget*) 
{
  QVBoxLayout* l = new QVBoxLayout;
  { QGroupBox* box = new QGroupBox("Sequencer Codes");
    QVBoxLayout* vl = new QVBoxLayout;
    { QGridLayout* layout = _elayout = new QGridLayout;
      layout->addWidget(new QLabel("Enable")   ,0,0,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Code")     ,0,1,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Type")     ,0,2,::Qt::AlignCenter);
      layout->addWidget(_pLabelGroup1 = new QLabel("Group")    
                                               ,0,3,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Describe") ,0,4,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Reporting"),0,5,::Qt::AlignCenter);
      for(unsigned i=0; i<MaxUserCodes; i++) {
        _seq_code[i].initialize(layout,i+1);
        ::QObject::connect(_seq_code[i]._enable, SIGNAL(toggled(bool)), this, SIGNAL(update_codes(bool)));
      }
      QWidget* w = new QWidget;
      w->setLayout(layout);
      QScrollArea* scroll = new QScrollArea;
      scroll->setHorizontalScrollBarPolicy(::Qt::ScrollBarAlwaysOff);
      scroll->setWidget(w);
      vl->addWidget(scroll); }
    box->setLayout(vl); 
    l->addWidget(box); }
  { QGroupBox* box = new QGroupBox("Global Codes");
    QVBoxLayout* vl = new QVBoxLayout;
    { QGridLayout* layout = new QGridLayout;
      layout->addWidget(new QLabel("Enable")   ,0,0,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Code")     ,0,1,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Type")     ,0,2,::Qt::AlignCenter);
      layout->addWidget(_pLabelGroup2 = new QLabel("Group")    
                                               ,0,3,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Describe") ,0,4,::Qt::AlignCenter);
      layout->addWidget(new QLabel("Reporting"),0,5,::Qt::AlignCenter);
      for(unsigned i=0; i<MaxGlobalCodes; i++) {
        _glb_code[i].initialize(layout,i+1);
      }
      vl->addLayout(layout); }
    box->setLayout(vl); 
    l->addWidget(box); }
  if (_defaults)
    l->addWidget(_defaults);

  {
    QHBoxLayout* hl = new QHBoxLayout;
    hl->addWidget(new QLabel("Readout Group Support"));
    _cbEnableReadGroup = new QComboBox;
    //_cbEnableReadGroup->addItem(QString().setNum(iGroup));
    _cbEnableReadGroup->addItem("Off");
    _cbEnableReadGroup->addItem("On");
    _cbEnableReadGroup->setCurrentIndex(0);        
    hl->addWidget(_cbEnableReadGroup);
    hl->addStretch();
    l->addLayout(hl);    
    ::QObject::connect(_cbEnableReadGroup, SIGNAL(currentIndexChanged(int)), this, SLOT(onEnableReadGroup(int)));    
  }
  
  onEnableReadGroup(0);
  return l;
}

void EvrEventCodeTable::update() { 
  for(unsigned i=0; i<MaxUserCodes; i++)
    _seq_code[i].update();
  for(unsigned i=0; i<MaxGlobalCodes; i++)
    _glb_code[i].update();
  _cbEnableReadGroup->update();        
}
void EvrEventCodeTable::flush() {
  for(unsigned i=0; i<MaxUserCodes; i++)
    _seq_code[i].flush();
  for(unsigned i=0; i<MaxGlobalCodes; i++)
    _glb_code[i].flush();
  if (_defaults)
    _defaults->flush();
}
void EvrEventCodeTable::enable(bool v) {
}

QStringList EvrEventCodeTable::code_names() const
{
  QStringList v;
  for(unsigned i=0; i<MaxUserCodes; i++)
    if (_seq_code[i].enabled())
      v << _seq_code[i].get_label();
  return v;
}

unsigned    EvrEventCodeTable::code_lookup(unsigned i) const
{
  for(unsigned j=0; j<MaxUserCodes; j++)
    if (_seq_code[j].enabled())
      if (!i--)
        return _seq_code[j].get_code();
  return -1;
}

unsigned    EvrEventCodeTable::code_index (unsigned c) const
{
  unsigned i=0;
  for(unsigned j=0; j<MaxUserCodes; j++)
    if (_seq_code[j].enabled()) {
      if (_seq_code[j].get_code()==c)
        return i;
      i++;
    }
  return -1;
}

unsigned    EvrEventCodeTable::ncodes() const
{
  return _ncodes;
}

const EventCodeType* EvrEventCodeTable::codes() const
{
  return reinterpret_cast<const EventCodeType*>(_code_buffer);
}

bool EvrEventCodeTable::enableReadoutGroup() const
{
  return (_cbEnableReadGroup->currentIndex() != 0);
}

void EvrEventCodeTable::onEnableReadGroup(int iIndex)
{
  printf("EvrEventCodeTable::onEnableReadGroup(%d)\n", iIndex); //!!debug
  bool bEnableReadGroup = (iIndex != 0);
  _pLabelGroup1->setVisible(bEnableReadGroup);
  _pLabelGroup2->setVisible(bEnableReadGroup);
  
  for(unsigned i=0; i<MaxUserCodes; i++)
    _seq_code[i].setGroupEnable(bEnableReadGroup);
    
  for(unsigned i=0; i<MaxGlobalCodes; i++)
    _glb_code[i].setGroupEnable(bEnableReadGroup);
    
  if (_pPulseTables != NULL)
    _pPulseTables->setReadGroupEnable(bEnableReadGroup);
}

void showLayoutItem(QLayoutItem* item, bool s )
{
  QWidget* w = item->widget();
  if (w) 
    w->setVisible(s);
  else {
    QLayout* l = item->layout();
    if (l) 
      for(int j=0; j<l->count(); j++)
        showLayoutItem(l->itemAt(j), s);
  }
}
