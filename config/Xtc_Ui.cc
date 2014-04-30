#include "pdsapp/config/Xtc_Ui.hh"

#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/GlobalCfg.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/BldInfo.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtCore/QDateTime>

#include <stdlib.h>
#include <fcntl.h>

#include <list>

using std::list;

static QString sname(const Pds::Src& src)
{
  QString r;
  switch(src.level()) {
  case Pds::Level::Source:
    r = QString(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src)));
    break;
  case Pds::Level::Reporter:
    r = QString(Pds::BldInfo::name(static_cast<const Pds::BldInfo&>(src)));
    break;
  default:
    break;
  }
  return r;
}

namespace Pds_ConfigDb {

  class DeviceIterator : public Pds::XtcIterator {
  public:
    DeviceIterator(QListWidget& list) : _list(list) {}
    ~DeviceIterator() {}
  public:
    int process(Xtc* xtc) {
      if (xtc->contains.id()==Pds::TypeId::Id_Xtc)
        iterate(xtc);
      else {
        QString name = sname(xtc->src);
        if (!name.isEmpty() && _list.findItems(name,::Qt::MatchExactly).size()==0)
          _list.addItem(new QListWidgetItem(name));
      }
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
      if (xtc->contains.id()==Pds::TypeId::Id_Xtc)
        iterate(xtc);
      else {
        QString name = sname(xtc->src);
        if (name == _name) {
          UTypeName          u = PdsDefs::utypeName(xtc->contains);
          if (PdsDefs::typeId(u)) {
            QTypeName          q = PdsDefs::qtypeName(u);
            const Pds::TypeId* t = PdsDefs::typeId(q);
            PdsDefs::ConfigType ctype = PdsDefs::configType(*t);
            if (ctype!=PdsDefs::NumberOf) {
              QString cname(PdsDefs::utypeName(ctype).c_str());
              int i;
              for(i=0; i<_list.count(); i++)
                if (_list.item(i)->text()==cname)
                  break;
              if (i==_list.count())
                _list.addItem(new QListWidgetItem(cname));
            }
            else
              printf("Failed to lookup %s (v%d)\n",
                     Pds::TypeId::name(xtc->contains.id()),
                     xtc->contains.version());
          }
        }
      }
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
      _cmp(cmp),
      _found(false) {}
    ~ConfigIterator()    {}
  public:
    int process(Xtc* xtc) {
      if (xtc->contains.id()==Pds::TypeId::Id_Xtc)
        iterate(xtc);
      else if (_cmp==QString(PdsDefs::utypeName(xtc->contains).c_str()) &&
               _dev==sname(xtc->src)) {
	Parameter::allowEdit(false);
        Dialog* d = new Dialog(_ui, _ui->lookup(xtc->contains), "xtc", xtc->payload(), xtc->sizeofPayload());
        d->exec();
        _found = true;
        return 0;
      }
      return 1;
    }
    bool found() const { return _found; }
  private:
    Xtc_Ui* _ui;
    QString _dev;
    QString _cmp;
    bool    _found;
  };
};

static void _copy_to_buffer(const Dgram* dg,
                            char*&       buffer)
{
  if (buffer) delete[] buffer;
  int size = sizeof(*dg)+dg->xtc.sizeofPayload();
  buffer = new char[size];
  memcpy(buffer, (char*)dg, size); 
}

using namespace Pds_ConfigDb;

Xtc_Ui::Xtc_Ui(QWidget* parent) :
  QWidget      (parent),
  _cfgdg_buffer(0),
  _l1adg_buffer(0),
  _fiter       (0),
  _icycle      (-1)
{
  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(_runInfo = new QLabel);
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
  setLayout(l);

  connect(_devlist, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(update_component_list()));
  connect(_cmplist, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(change_component()));
}

Xtc_Ui::~Xtc_Ui()
{
  if (_cfgdg_buffer)
    delete[] _cfgdg_buffer;
  if (_l1adg_buffer)
    delete[] _l1adg_buffer;
  if (_fiter)
    delete _fiter;
}

void Xtc_Ui::set_file(QString fname)
{
  _cycle.clear();
  _icycle = 0;

  int fd = ::open(qPrintable(fname),O_LARGEFILE,O_RDONLY);
  if (fd == -1) {
    char buff[256];
    sprintf(buff,"Error opening file %s\n",qPrintable(fname));
    perror(buff);
    return;
  }

  _fiter = new Pds::XtcFileIterator(fd,0x2000000);

  Pds::XtcFileIterator& iter = *_fiter;
  Pds::Dgram* dg;
  
  while((dg = iter.next())) {
    if (dg->seq.service()==Pds::TransitionId::Configure) {
      _copy_to_buffer(dg, _cfgdg_buffer);
      _cycle.push_back(dg);
    }
    else if (dg->seq.service()==Pds::TransitionId::BeginCalibCycle) {
      _cycle.push_back(dg);
    }
    else if (dg->seq.service()==Pds::TransitionId::L1Accept) {
      _copy_to_buffer(dg, _l1adg_buffer);
      break;
    }
  }
  
  const Pds::ClockTime& time = reinterpret_cast<const Pds::Dgram*>(_cfgdg_buffer)->seq.clock();
  QDateTime datime; datime.setTime_t(time.seconds());
  QString info = QString("%1.%2")
    .arg(datime.toString("MMM dd,yyyy hh:mm:ss"))
    .arg(QString::number(time.nanoseconds()/1000000),3,QChar('0'));

  _runInfo->setText(info);

  update_device_list();
}

void Xtc_Ui::next_cycle()
{
  int icycle = _icycle+1;
  if (icycle>=int(_cycle.size())) {
    Pds::XtcFileIterator& iter = *_fiter;
    Pds::Dgram* dg;
    while((dg = iter.next())) {
      if (dg->seq.service()==Pds::TransitionId::BeginCalibCycle) {
	_cycle.push_back(dg);
	if (icycle<int(_cycle.size())) break;
      }
    }
  }

  if (icycle>=int(_cycle.size())) return;

  emit set_cycle(_icycle = icycle);
  _copy_to_buffer(_cycle[icycle], _cfgdg_buffer);
  update_device_list();
}

void Xtc_Ui::prev_cycle()
{
  int icycle = _icycle-1;
  if (icycle<0) return;

  emit set_cycle(_icycle = icycle);
  _copy_to_buffer(_cycle[icycle], _cfgdg_buffer);
  update_device_list();
}

//  Parse XTC for all devices
void Xtc_Ui::update_device_list()
{
  disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  _devlist->clear();

  DeviceIterator iter(*_devlist);
  iter.iterate(&reinterpret_cast<Pds::Dgram*>(_cfgdg_buffer)->xtc);
  iter.iterate(&reinterpret_cast<Pds::Dgram*>(_l1adg_buffer)->xtc);

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
    iter.iterate(&reinterpret_cast<Pds::Dgram*>(_cfgdg_buffer)->xtc);
    iter.iterate(&reinterpret_cast<Pds::Dgram*>(_l1adg_buffer)->xtc);
  }

  if (ok_change) connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
}

//  View component
void Xtc_Ui::change_component()
{
  ConfigIterator iter(this, 
                      _devlist->currentItem()->text(), 
                      _cmplist->currentItem()->text());
  iter.iterate(&reinterpret_cast<Pds::Dgram*>(_l1adg_buffer)->xtc);
  if (!iter.found())
    iter.iterate(&reinterpret_cast<Pds::Dgram*>(_cfgdg_buffer)->xtc);
}

Serializer& Xtc_Ui::lookup(const Pds::TypeId& stype)
{ 
  return *_dict .lookup(stype);
}    
