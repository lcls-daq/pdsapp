#include "pdsapp/config/TriggerConfigP.hh"
#include "pdsapp/config/L0Select.hh"
#include "pds/config/TriggerConfigType.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QTabWidget>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>

#define ADDTAB(p,title) {                       \
    QWidget* w = new QWidget;                   \
    w->setLayout(p->initialize(0));             \
    tab->addTab(w,title); }

namespace Pds_ConfigDb {

  class TriggerConfigP::Private_Data : public Parameter {
  public:
    Private_Data() :
      Parameter(NULL)
    {}
    ~Private_Data()
    {}
  public:
    QLayout* initialize(QWidget*) {
      QHBoxLayout* layout = new QHBoxLayout;
      QTabWidget* tab = new QTabWidget;
      _l0Select = new L0Select;
      ADDTAB(_l0Select,"L0Select");
      layout->addWidget(tab);
      return layout;
    }
    void     update    () { _l0Select ->update(); }
    void     flush     () { _l0Select ->flush (); }
    void     enable    (bool) {}
  public:
    int pull(void *from) {
      const TriggerConfigType& tc = *reinterpret_cast<const TriggerConfigType*>(from);
      _l0Select->pull(tc.l0Select()[0]);
      return tc._sizeof();
    }
    int push(void *to) {
      char* p = new char[Pds::TriggerData::L0SelectV1::_sizeof()];
      L0SelectType* pc = reinterpret_cast<L0SelectType*>(p);
      _l0Select->push(pc);
      TriggerConfigType& tc = *new(to) TriggerConfigType(0, 1, NULL, pc);
      int sz = tc._sizeof();
      delete p;
      return sz;
    }

    int dataSize() const {
      char* p = new char[0x100000];
      L0SelectType l0;
      TriggerConfigType& tc = *new(p) TriggerConfigType(0, 1, NULL, &l0);
      int sz = tc._sizeof();
      delete p;
      return sz;
    }
    bool validate() {
      return _l0Select->validate();
    }
  private:
    L0Select* _l0Select;
  };

};


using namespace Pds_ConfigDb;

TriggerConfigP::TriggerConfigP():
  Serializer("Xpm_Config"), _private_data(new Private_Data)
{
  pList.insert(_private_data);
}

TriggerConfigP::~TriggerConfigP()
{
  delete _private_data;
}

int TriggerConfigP::readParameters(void *from)
{
  return _private_data->pull(from);
}

int TriggerConfigP::writeParameters(void *to)
{
  return _private_data->push(to);
}

int TriggerConfigP::dataSize() const
{
  return _private_data->dataSize();
}

bool TriggerConfigP::validate() 
{
  return _private_data->validate();
}
