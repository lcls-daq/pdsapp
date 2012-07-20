#include "pdsapp/config/Xtc_Ui.hh"

#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/GlobalCfg.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Dgram.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>

#include <list>

using std::list;

namespace Pds_ConfigDb {

  class DeviceIterator : public Pds::XtcIterator {
  public:
    DeviceIterator(QListWidget& list) : _list(list) {}
    ~DeviceIterator() {}
  public:
    int process(Xtc* xtc) {
      if (xtc->src.level()==Pds::Level::Source) {
        QString name(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(xtc->src)));
        if (_list.findItems(name,::Qt::MatchExactly).size()==0)
          _list.addItem(new QListWidgetItem(name));
      }
      else if (xtc->contains.id()==Pds::TypeId::Id_Xtc)
        iterate(xtc);
      return 1;
    }
  private:
    QListWidget& _list;
  };

  class ComponentIterator : public Pds::XtcIterator {
  public:
    ComponentIterator(QListWidget& list,QString name) : 
      _list(list), _name(name) {}
    ~ComponentIterator() {}
  public:
    int process(Xtc* xtc) {
      if (xtc->src.level()==Pds::Level::Source &&
          QString(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(xtc->src)))==_name) {
        UTypeName          u = PdsDefs::utypeName(xtc->contains);
        if (PdsDefs::typeId(u)) {
          QTypeName          q = PdsDefs::qtypeName(u);
          const Pds::TypeId* t = PdsDefs::typeId(q);
          PdsDefs::ConfigType ctype = PdsDefs::configType(*t);
          if (ctype!=PdsDefs::NumberOf)
            _list.addItem(new QListWidgetItem(QString(PdsDefs::utypeName(ctype).c_str())));
          else
            printf("Failed to lookup %s (v%d)\n",
                   Pds::TypeId::name(xtc->contains.id()),
                   xtc->contains.version());
        }
      }
      else if (xtc->contains.id()==Pds::TypeId::Id_Xtc)
        iterate(xtc);
      return 1;
    }
  private:
    QListWidget& _list;
    QString      _name;
  };

  class ConfigIterator : public XtcIterator {
  public:
    ConfigIterator(Xtc_Ui* ui,
                   const QString& dev,
                   const QString& cmp) :
      _ui (ui),
      _dev(dev),
      _cmp(cmp)    {}
    ~ConfigIterator()    {}
  public:
    int process(Xtc* xtc) {
      if (xtc->contains.id()==Pds::TypeId::Id_Xtc)
        iterate(xtc);
      else if (_cmp==QString(PdsDefs::utypeName(xtc->contains).c_str()) &&
               _dev==QString(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(xtc->src)))) {
        Dialog* d = new Dialog(_ui, _ui->lookup(xtc->contains), xtc->payload(), xtc->sizeofPayload());
        d->exec();
        return 0;
      }
      return 1;
    }
  private:
    Xtc_Ui* _ui;
    QString _dev;
    QString _cmp;
  };
};

using namespace Pds_ConfigDb;

Xtc_Ui::Xtc_Ui(QWidget* parent,
               Dgram&   dgram) :
  QDialog(parent),
  _dgram (dgram)
{
  setWindowTitle("Read Xtc");
  setAttribute(::Qt::WA_DeleteOnClose, true);
  setModal(false);
  Parameter::allowEdit(false);

  QPushButton* closeB = new QPushButton("Close");

  QVBoxLayout* l = new QVBoxLayout;
  { QHBoxLayout* layout = new QHBoxLayout;
    { QVBoxLayout* layout1 = new QVBoxLayout;
      layout1->addWidget(new QLabel("Device", this));
      layout1->addWidget(_devlist = new QListWidget(this));
      layout->addLayout(layout1); }
    { QVBoxLayout* layout1 = new QVBoxLayout;
      layout1->addWidget(new QLabel("Component", this));
      layout1->addWidget(_cmplist = new QListWidget(this));
      layout->addLayout(layout1); }
    l->addLayout(layout); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(closeB);
    layout->addStretch();
    l->addLayout(layout); }
  setLayout(l);

  connect(_devlist, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(update_component_list()));
  connect(_cmplist, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(change_component()));
  connect(closeB  , SIGNAL(clicked()), this, SLOT(close()));

  update_device_list();
}

//  Parse XTC for all devices
void Xtc_Ui::update_device_list()
{
  bool ok = disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  _devlist->clear();

  DeviceIterator iter(*_devlist);
  iter.iterate(&_dgram.xtc);

  connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  update_component_list();
}

//  Parse XTC for all components belonging to this device
void Xtc_Ui::update_component_list()
{
  bool ok_change = disconnect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
  _cmplist->clear();

  QListWidgetItem* item = _devlist->currentItem();
  if (item) {
    ComponentIterator iter(*_cmplist,item->text());
    iter.iterate(&_dgram.xtc);
  }

  if (ok_change) connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
}

//  View component
void Xtc_Ui::change_component()
{
  ConfigIterator iter(this, 
                      _devlist->currentItem()->text(), 
                      _cmplist->currentItem()->text());
  iter.iterate(&_dgram.xtc);
}

Serializer& Xtc_Ui::lookup(const Pds::TypeId& stype)
{ 
  return *_dict .lookup(stype);
}    
