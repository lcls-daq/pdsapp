#include "pdsapp/config/EvrEventCodeTable.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/EvrSeqEventDesc.hh"
#include "pdsapp/config/EvrGlbEventDesc.hh"

//#include "pds/config/SeqConfigType.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>

#include <stdio.h>

static const unsigned DefaultLo = 67;
static const unsigned DefaultHi = 74;
static const unsigned MaxUserCodes      = Pds_ConfigDb::EvrEventCodeTable::MaxCodes;
static const unsigned MinUserCodes      = 8;
static const unsigned MaxGlobalCodes    = 4;

static void showLayoutItem(QLayoutItem* item, bool show);

namespace Pds_ConfigDb {

  class FixedEventCode {
  public:
    FixedEventCode(const char* label) :
      _label(new QLineEdit(label)),
      _code (new QLabel) {}
  public:
    QLayout* initialize(QGridLayout* grid, int row) {
      QCheckBox* cb = new QCheckBox; cb->setChecked(true); cb->setEnabled(false);
      grid->addWidget(cb    , row, 0, Qt::AlignCenter);
      grid->addWidget(_code , row, 1, Qt::AlignCenter);
      grid->addWidget(_label, row, 3, Qt::AlignCenter);
      _label->setEnabled(false);
      return grid;
    }
    void set_code(unsigned n) { _code->setText(QString::number(n,10)); }
    unsigned get_code() const { return _code->text().toInt(); }
  private:
    QLineEdit* _label;
    QLabel* _code;
  };

};

using namespace Pds_ConfigDb;


EvrEventCodeTable::EvrEventCodeTable(EvrPulseTables* pPulseTables) : 
  Parameter(NULL),
  _pPulseTables(pPulseTables),
  _range_lo(NULL,DefaultLo,0,255),
  _range_hi(NULL,DefaultHi,0,255),
  _ncodes     (0),
  _pLabelGroup1     (NULL),
  _pLabelGroup2     (NULL),
  _cbEnableReadGroup(NULL),
  _code_buffer(new char[(MaxUserCodes+MaxGlobalCodes)
                        *sizeof(EvrConfigType::EventCodeType)])                        
{
  _seq_code = new EvrSeqEventDesc[MaxUserCodes];
  _glb_code = new EvrGlbEventDesc[MaxGlobalCodes];
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

  unsigned max_seq = 0;
  unsigned min_seq = 256;
  unsigned nglb=0;

  bool bEneableReadoutGroup = false;
  for(unsigned i=0; i<cfg.neventcodes(); i++) {
    const EvrConfigType::EventCodeType& e = cfg.eventcode(i);
    if (e.readoutGroup() > 1)
      bEneableReadoutGroup = true;
    if (EvrGlbEventDesc::global_code(e.code())) {
      _glb_code[nglb++].pull(e);
      continue;
    }
    if (e.code() > max_seq)
      max_seq = e.code();
    if (e.code() < min_seq)
      min_seq = e.code();
  }
  
  _cbEnableReadGroup->setCurrentIndex(bEneableReadoutGroup? 1 : 0);      

  if ( min_seq == 256 )
  {
    _range_lo.value = DefaultLo;
    _range_hi.value = DefaultHi;
    return;
  }
  
  _range_lo.value = min_seq;
  
  if (max_seq < _range_lo.value+MaxUserCodes)
    _range_hi.value = max_seq;  
  else 
    _range_hi.value = max_seq + MaxUserCodes - 1;
    
  for(unsigned i=0; i<cfg.neventcodes(); i++) {
    const EvrConfigType::EventCodeType& e = cfg.eventcode(i);
    if (EvrGlbEventDesc::global_code(e.code())) 
      continue;      
    
    if ( e.code() <= _range_hi.value )
    _seq_code[e.code()-_range_lo.value].pull(e);
  }
  update_range();      
}

bool EvrEventCodeTable::validate() {

  EvrConfigType::EventCodeType* codep = 
    reinterpret_cast<EvrConfigType::EventCodeType*>(_code_buffer);

  //  Every "latch" type should have a partner un-"latch"
  //  All enabled codes should be within range
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
        return false;
      }
    }

  for(unsigned i=0; i<MaxUserCodes; i++)
    if (_seq_code[i].enabled() && _seq_code[i].get_code()<=_range_hi.value)
      _seq_code[i].push(codep++);

  for(unsigned i=0; i<MaxGlobalCodes; i++)
    if (_glb_code[i].enabled())
      _glb_code[i].push(codep++);

  _ncodes = codep - reinterpret_cast<EvrConfigType::EventCodeType*>(_code_buffer);

  { const EvrConfigType::EventCodeType* pcode = reinterpret_cast<const EvrConfigType::EventCodeType*>(_code_buffer);
    for(int i=0; i<int(_ncodes)-1; i++)
      for(int j=i+1; j<int(_ncodes); j++) {
        if (pcode[j].code() == pcode[i].code()) {
          QString msg = QString("Event code %1 is specified multiple times").arg(pcode[i].code());
          QMessageBox::warning(0,"Input Error",msg);
          return false;
        }
      }
  }
    
  return true;
}

QLayout* EvrEventCodeTable::initialize(QWidget*) 
{
  QVBoxLayout* l = new QVBoxLayout;
  { QGroupBox* box = new QGroupBox("Sequencer Codes");
    QVBoxLayout* vl = new QVBoxLayout;
    { QHBoxLayout* hl = new QHBoxLayout;
      hl->addWidget(new QLabel("Event Code Range"));
      hl->addLayout(_range_lo.initialize(0)); 
      hl->addLayout(_range_hi.initialize(0)); 
      hl->addStretch();
      vl->addLayout(hl); }
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
      vl->addLayout(layout); }
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

  _range_lo.widget()->setMaximumWidth(40);
  _range_hi.widget()->setMaximumWidth(40);
  if (allowEdit()) {
  ::QObject::connect(_range_lo._input, SIGNAL(editingFinished()), this, SLOT(update_range()));
  ::QObject::connect(_range_hi._input, SIGNAL(editingFinished()), this, SLOT(update_range()));
  }
  update_range();
  
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
  _range_lo.update();
  _range_hi.update();
  for(unsigned i=0; i<MaxUserCodes; i++)
    _seq_code[i].update();
  for(unsigned i=0; i<MaxGlobalCodes; i++)
    _glb_code[i].update();
  _cbEnableReadGroup->update();        
}
void EvrEventCodeTable::flush() {
  _range_lo.flush();
  _range_hi.flush();
  for(unsigned i=0; i<MaxUserCodes; i++)
    _seq_code[i].flush();
  for(unsigned i=0; i<MaxGlobalCodes; i++)
    _glb_code[i].flush();
}
void EvrEventCodeTable::enable(bool v) {
}

void EvrEventCodeTable::update_range() 
{
  unsigned c = _range_lo.value;
  for(unsigned i=0; i<MaxUserCodes; i++)
    _seq_code[i].set_code(c++);

  if ((_range_hi.value > _range_lo.value+1) &&
      (_range_hi.value < _range_lo.value+MaxUserCodes)) {

    int len = _range_hi.value - _range_lo.value + 1;

    for(int i=0; i<_elayout->count(); i++) {
      int row, col, rowspan, colspan;
      _elayout->getItemPosition(i, &row, &col, &rowspan, &colspan);
      bool hide = row>len;
      if (col<2 || hide)
        showLayoutItem(_elayout->itemAt(i), !hide);
    }
  }    
        
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

const EvrConfigType::EventCodeType* EvrEventCodeTable::codes() const
{
  return reinterpret_cast<const EvrConfigType::EventCodeType*>(_code_buffer);
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
