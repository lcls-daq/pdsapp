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

namespace Pds_ConfigDb {
  namespace Trigger {
    class FixedRate : public QComboBox {
    public:
      FixedRate() 
      {
        addItem("929 kHz");
        addItem("186 kHz");
        addItem("92.9kHz");
        addItem("9.29kHz");
        addItem("929 Hz");
        addItem("92.9Hz");
        addItem("9.29Hz");
        addItem(".929Hz");
      }
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
  switch(cfg.rateSelect()) {
  case L0SelectType::_FixedRate:
    _tab->setCurrentIndex(0);
    _fixedRate->setCurrentIndex(cfg.fixedRate());
  default:
    break;
  }
}

int L0Select::push(L0SelectType* to) const
{
  L0SelectType& q = *new(to) L0SelectType(L0SelectType::FixedRate(_fixedRate->currentIndex()),
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
  { _fixedRate = new Trigger::FixedRate;
    _tab->addTab(_fixedRate,"Fixed Rate"); }
  layout->addWidget(_tab);
  return layout;
}

void L0Select::update() { 
}
void L0Select::flush() {
}
void L0Select::enable(bool v) {
}

