#include "pdsapp/config/L0Select.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsdata/psddl/trigger.ddl.h"

#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>
#include <QtGui/QScrollArea>

#include <stdio.h>

using namespace Pds_ConfigDb;

enum { FixedRate_S, ACRate_S, SeqSelect_S, EventCode_S };

namespace Pds_ConfigDb {
  namespace Trigger {
    class FixedRate : public QComboBox {
    public:
      FixedRate() 
      {
        addItem("929 kHz");
        addItem("71.4kHz");
        addItem("10.2kHz");
        addItem("1.02kHz");
        addItem("102 Hz");
        addItem("10.2Hz");
        addItem("1.02Hz");
      }
    };
    class ACRate : public QComboBox {
    public:
      ACRate() 
      {
        addItem("60 Hz");
        addItem("30 Hz");
        addItem("10 Hz");
        addItem(" 5 Hz");
        addItem(" 1 Hz");
      }
    };
    class SeqSelect : public QWidget {
    public:
      SeqSelect() :
        _seq("Seq#", 0, 0, 18),
        _bit("Bit#", 0, 0, 15)
      { QHBoxLayout* l = new QHBoxLayout;
        l->addLayout( _seq.initialize(this) );
        l->addLayout( _bit.initialize(this) );
        setLayout(l); }
    public:
      unsigned seq() const { return _seq.value; }
      unsigned bit() const { return _bit.value; }
      void flush () {
        _seq.flush();
        _bit.flush();
      }
      void update() {
        _seq.update();
        _bit.update();
      }
      void set(unsigned sv, 
               unsigned bv) {
        _seq.value = sv;
        _bit.value = bv;
      }
    private:
      NumericInt<unsigned> _seq;
      NumericInt<unsigned> _bit;
    };
    class CodeSelect : public QComboBox {
    public:
      CodeSelect() {
        for(unsigned i=1; i<256; i++) {
          switch(i) {
          case  40: addItem(" 40 (120 Hz)"); break;
          case  41: addItem(" 41 ( 60 Hz)"); break;
          case  42: addItem(" 42 ( 30 Hz)"); break;
          case  43: addItem(" 43 ( 10 Hz)"); break;
          case  44: addItem(" 44 (  5 Hz)"); break;
          case  45: addItem(" 45 (  1 Hz)"); break;
          case  46: addItem(" 46 ( 0.5Hz)"); break;
          case 140: addItem("140 (120 Hz beam)"); break;
          case 141: addItem("141 ( 60 Hz beam)"); break;
          case 142: addItem("142 ( 30 Hz beam)"); break;
          case 143: addItem("143 ( 10 Hz beam)"); break;
          case 144: addItem("144 (  5 Hz beam)"); break;
          case 145: addItem("145 (  1 Hz beam)"); break;
          case 146: addItem("146 ( 0.5Hz beam)"); break;
          default:  addItem(QString("%1").arg(i)); break;
          };
        }
      }
    public:
      unsigned get() const { return currentIndex()+1; }
      void     set(unsigned v) { setCurrentIndex(v-1); }
    };
  }
}

L0Select::L0Select() :
  Parameter(NULL)
{
}

L0Select::~L0Select()
{
}

void L0Select::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);
}

void L0Select::pull(const L0SelectType& cfg) 
{ 
  _tab->setCurrentIndex( int(cfg.rateSelect()) );
  _fixedRate ->setCurrentIndex(cfg.fixedRate());
  _acRate    ->setCurrentIndex(cfg.powerSyncRate());
  _seqSelect ->set(cfg.controlSeqNum(),
                   cfg.controlSeqBit());
  _codeSelect->set(cfg.eventCode());
}

int L0Select::push(L0SelectType* to) const
{
  L0SelectType& q = *new (to) L0SelectType(L0SelectType::RateSelect(_tab->currentIndex()),
                                           L0SelectType::FixedRate(_fixedRate->currentIndex()),
                                           L0SelectType::PowerSyncRate(_acRate->currentIndex()), 0x1,
                                           _seqSelect->seq(), _seqSelect->bit(),
                                           _codeSelect->get(),
                                           0, // Partition
                                           L0SelectType::_DontCare);
  return q._sizeof();
}

bool L0Select::validate() { return true; }

#define ADDTAB(p,title) {                       \
    QWidget* w = new QWidget;                   \
    w->setLayout(p->initialize(0));             \
    _tab->addTab(w,title); }

QLayout* L0Select::initialize(QWidget*) 
{
  QHBoxLayout* layout = new QHBoxLayout;
  _tab = new QTabWidget;
  { _fixedRate  = new Trigger::FixedRate;
    _tab->addTab(_fixedRate,"Fixed Rate"); }
  { _acRate     = new Trigger::ACRate;
    _tab->addTab(_acRate,"AC Rate"); }
  { _seqSelect  = new Trigger::SeqSelect;
    _tab->addTab(_seqSelect,"Sequence"); }
  { _codeSelect = new Trigger::CodeSelect;
    _tab->addTab(_seqSelect,"EventCode"); 
    _tab->setTabEnabled( EventCode_S, false); }  // Hide LCLS-I until synchronized
  layout->addWidget(_tab);
  return layout;
}

void L0Select::update() { _seqSelect->update(); }
void L0Select::flush () { _seqSelect->flush(); }
void L0Select::enable(bool v) {
}

