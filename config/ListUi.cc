#include "pdsapp/config/ListUi.hh"

#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/Parameters.hh"

#include "pds/config/DbClient.hh"
#include "pds/config/PdsDefs.hh"
#include "pds/config/DeviceEntry.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>

#include <fstream>
using std::ifstream;

using namespace Pds_ConfigDb;

ListUi::ListUi(DbClient& path) :
  QWidget (0),
  _db     (path),
  _lenbuff(0x100000),
  _xtcbuff(new char[_lenbuff])
{
  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Key",this));
    layout1->addWidget(_keylist = new QListWidget(this));
    _keylist->setMinimumWidth(400);
    layout->addLayout(layout1); }
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Devices",this));
    layout1->addWidget(_devlist = new QListWidget(this));
    layout->addLayout(layout1); }
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Configuration Type",this));
    layout1->addWidget(_xtclist = new QListWidget(this));
    layout->addLayout(layout1); }
  setLayout(layout);

  // populate key list
  std::list<Key> keys = _db.getKeys();
  for(std::list<Key>::const_iterator it=keys.begin();
      it!=keys.end(); it++) {
    time_t t(it->time);
    char keynum[10];
    sprintf(keynum,"%08x",it->key);
    QString entry = QString("%1  [%2]")
      .arg(keynum)
      .arg(QString(ctime(&t)).remove('\n'));
    if (it->name.size())
      entry += "  [" + QString(it->name.c_str()) + "]";
    *new QListWidgetItem(entry,_keylist);
  }

  connect(_keylist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
  connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_xtc_list()));
  connect(_xtclist, SIGNAL(itemSelectionChanged()), this, SLOT(view_xtc()));
}

ListUi::~ListUi()
{
  delete[] _xtcbuff;
}

void ListUi::update_device_list()
{
  bool ok = disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_xtc_list()));
  _devlist->clear();
  _devices .clear();
  QListWidgetItem* item = _keylist->currentItem();
  if (item) {
    bool ok;
    std::list<KeyEntry> entries = _db.getKey(item->text().split(' ')[0].toUInt(&ok,16));

    for(std::list<KeyEntry>::const_iterator it=entries.begin();
        it!=entries.end(); it++) {

      bool lfound=false;
      for(unsigned i=0; i<_devices.size(); i++)
        if (_devices[i]==it->source) {
          lfound=true;
          break;
        }
      if (!lfound) {
        DeviceEntry phy(it->source);
        const Pds::Src& src = phy;
        QString name = (src.level()==Pds::Level::Source) ?
          QString(Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(src))) :
          QString(Pds::Level::name(src.level()));
        *new QListWidgetItem(name,_devlist);

        _devices.push_back(it->source);
      }
    }
  }
  if (ok) connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_xtc_list()));
  update_xtc_list();
}

void ListUi::update_xtc_list()
{
  bool ok = disconnect(_xtclist, SIGNAL(itemSelectionChanged()), this, SLOT(view_xtc()));
  _xtclist->clear();
  _types   .clear();
  QListWidgetItem* item = _devlist->currentItem();
  if (item) {
    bool lok;
    std::list<KeyEntry> entries = _db.getKey(_keylist->currentItem()->text().split(' ')[0].toUInt(&lok,16));
    for(std::list<KeyEntry>::const_iterator it=entries.begin();
        it!=entries.end(); it++)
      if (it->source == _devices[_devlist->currentRow()]) {
        *new QListWidgetItem(Pds::TypeId::name(it->xtc.type_id.id()),_xtclist);
        _types.push_back(it->xtc.type_id);
      }
  }
  if (ok) connect(_xtclist, SIGNAL(itemSelectionChanged()), this, SLOT(view_xtc()));
}

void ListUi::view_xtc()
{
  int len=0;
  bool ok;
  unsigned key     = _keylist->currentItem()->text().split(' ')[0].toUInt(&ok,16);
  std::list<KeyEntryT> entry = _db.getKeyT(key);
  for(std::list<KeyEntryT>::iterator it=entry.begin();
      it!=entry.end(); it++)
    if (it->source                  == _devices[_devlist->currentRow()] &&
        it->xtc.xtc.type_id.value() == _types  [_xtclist->currentRow()].value()) {
      len = _db.getXTC(it->xtc);
      if (len<0)
        return;
      if (int(_lenbuff) < len) {
        delete[] _xtcbuff;
        _xtcbuff = new char[_lenbuff=len];
      }
      len = _db.getXTC(it->xtc,_xtcbuff,_lenbuff);
    }

  Parameter::allowEdit(false);
  Dialog* d = new Dialog(_xtclist, 
                         *_dict.lookup(_types[_xtclist->currentRow()]),
                         "xtc", _xtcbuff, len, false);
  d->setAttribute(::Qt::WA_DeleteOnClose, true);
  d->show();
}
