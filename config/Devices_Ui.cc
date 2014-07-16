#include "pdsapp/config/Devices_Ui.hh"

#include "pdsapp/config/DetInfoDialog_Ui.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Device.hh"
#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/Parameters.hh"

#include "pds/config/DbClient.hh"
#include "pds/config/PdsDefs.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtGui/QInputDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>

#include <list>
#include <string>
#include <iostream>
#include <sys/stat.h>

using std::list;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include <libgen.h>

using namespace Pds_ConfigDb;

Devices_Ui::Devices_Ui(QWidget* parent,
           Experiment& expt,
           bool edit) :
  QGroupBox("Devices Configuration", parent),
  _expt    (expt),
  _expert_mode(false),
  _edit       (edit)
{
  QVBoxLayout* vbox = new QVBoxLayout;
  { QMenuBar* menu_bar = new QMenuBar;
    QMenu* mode_menu = new QMenu("Options");
    mode_menu->addAction("Expert mode", this, SLOT(expert_mode()));
    mode_menu->addAction("User mode", this, SLOT(user_mode()));
    menu_bar->addMenu(mode_menu);
    vbox->addWidget(menu_bar); }
  { QHBoxLayout* layout = new QHBoxLayout;
    { QVBoxLayout* layout1 = new QVBoxLayout;
      layout1->addWidget(new QLabel("Device", this));
      layout1->addWidget(_devlist = new QListWidget(this));
      { QHBoxLayout* layout1a = new QHBoxLayout;
	layout1a->addWidget(_devnewedit   = new QLineEdit(this));
	layout1a->addWidget(_devnewbutton = new QPushButton("New", this));
	layout1->addLayout(layout1a); }
      { QHBoxLayout* layout1a = new QHBoxLayout;
	layout1a->addWidget(_deveditbutton = new QPushButton("Edit", this));
	layout1a->addStretch();
	layout1a->addWidget(_devrembutton = new QPushButton("Remove", this));
	layout1->addLayout(layout1a); }
      layout->addLayout(layout1); }
    { QVBoxLayout* layout1 = new QVBoxLayout;
      layout1->addWidget(new QLabel("Config Name", this));
      layout1->addWidget(_cfglist = new QListWidget(this));
      { QGridLayout* layout1a = new QGridLayout;
	layout1a->addWidget(_cfgnewedit   = new QLineEdit(this), 0, 0 );
	layout1a->addWidget(_cfgnewbutton = new QPushButton("New", this), 0, 1 );
	layout1a->addWidget(_cfgcpyedit   = new QLineEdit(this), 1, 0 );
	layout1a->addWidget(_cfgcpybutton = new QPushButton("Copy To", this), 1, 1 );
	layout1a->addWidget(_cfgrembutton = new QPushButton("Remove"), 0, 2, 2, 1 );
	layout1->addLayout(layout1a); }
      layout->addLayout(layout1); }
    { QVBoxLayout* layout1 = new QVBoxLayout;
      layout1->addWidget(new QLabel("Component", this));
      layout1->addWidget(_cmplist = new QListWidget(this));
      { QHBoxLayout* layout1a = new QHBoxLayout;
	QLabel* label = new QLabel("Add", this);
	label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter );
	layout1a->addWidget(label);
	layout1a->addWidget(_cmpaddlist = new QComboBox(this));
	layout1->addLayout(layout1a); }
      { QHBoxLayout* layout1a = new QHBoxLayout;
	QLabel* label = new QLabel("Remove", this);
	label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter );
	layout1a->addWidget(label);
	layout1a->addWidget(_cmpremlist = new QComboBox(this));
	layout1->addLayout(layout1a); }
      layout->addLayout(layout1); }
    vbox->addLayout(layout); }
  setLayout(vbox);

  if (edit) {
    connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_config_list()));
    connect(_deveditbutton, SIGNAL(clicked()), this, SLOT(edit_device()));
    connect(_devrembutton, SIGNAL(clicked()), this, SLOT(remove_device()));
    connect(_devnewbutton, SIGNAL(clicked()), this, SLOT(new_device()));
    connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
    connect(_cfgnewbutton, SIGNAL(clicked()), this, SLOT(new_config()));
    connect(_cfgcpybutton, SIGNAL(clicked()), this, SLOT(copy_config()));
    connect(_cfgrembutton, SIGNAL(clicked()), this, SLOT(remove_config()));
    connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
    connect(_cmpaddlist, SIGNAL(activated(const QString&)), this, SLOT(add_component(const QString&)));
    connect(_cmpremlist, SIGNAL(activated(const QString&)), this, SLOT(remove_component(const QString&)));
  }
  else {
    connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_config_list()));
    _deveditbutton->setText("View");
    connect(_deveditbutton, SIGNAL(clicked()), this, SLOT(view_device()));
    _devnewbutton ->setEnabled(false);
    connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
    _cfgnewbutton ->setEnabled(false);
    _cfgcpybutton ->setEnabled(false);
    connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(view_component()));
    _cmpaddlist   ->setEnabled(false);
    _cmpremlist   ->setEnabled(false);
  }
  update_device_list();
}

const Device* Devices_Ui::_device_c() const
{
  const Experiment& expt(_expt);
  QListWidgetItem* item = _devlist->currentItem();
  return item ? expt.device(string(qPrintable(item->text()))) : 0;
}

Device* Devices_Ui::_device() const
{
  QListWidgetItem* item = _devlist->currentItem();
  return item ? _expt.device(string(qPrintable(item->text()))) : 0;
}

void Devices_Ui::db_update()
{
  update_device_list();
}

void Devices_Ui::update_device_list()
{
  bool ok = disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_config_list()));
  _devlist->clear();
  for(list<Device>::const_iterator iter=_expt.devices().begin();
      iter!=_expt.devices().end(); iter++)
    *new QListWidgetItem(iter->name().c_str(), _devlist);
  if (ok) connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_config_list()));
  update_config_list();
}

void Devices_Ui::edit_device()
{
  Device* device(_device());
  if (device) {
    list<Pds::Src> dlist;
    for(list<DeviceEntry>::const_iterator iter = device->src_list().begin();
  iter!=device->src_list().end(); iter++) {
      dlist.push_back(*iter);
    }
    DetInfoDialog_Ui* dialog=new DetInfoDialog_Ui(this, dlist);
    if (dialog->exec()) {
      device->src_list().clear();
      for(list<Pds::Src>::const_iterator iter=dialog->src_list().begin();
    iter!=dialog->src_list().end(); iter++)
  device->src_list().push_back(DeviceEntry(*iter));
    }
    delete dialog;
  }
}

void Devices_Ui::view_device()
{
  const Device* device(_device_c());
  if (device) {
    list<Pds::Src> dlist;
    for(list<DeviceEntry>::const_iterator iter = device->src_list().begin();
        iter!=device->src_list().end(); iter++) {
      dlist.push_back(*iter);
    }
    DetInfoDialog_Ui* dialog=new DetInfoDialog_Ui(this, dlist, false);
    dialog->exec();
    delete dialog;
  }
}

void Devices_Ui::remove_device()
{
  Device* device(_device());
  if (device) {
    _expt.remove_device(*device);
    update_device_list();
    emit db_changed();
  }
}

void Devices_Ui::new_device()
{
  string device(_devnewedit->text().toAscii());
  _devnewedit->clear();
  if (device.empty())
    cout << "New device name \"" << device << "\" rejected" << endl;
  else {
    for(list<Device>::const_iterator iter=_expt.devices().begin();
  iter!=_expt.devices().end(); iter++)
      if (iter->name()==device) {
  cout << "New device name \"" << device << "\" rejected" << endl;
  return;
      }
    list<Pds::Src> dlist;
    DetInfoDialog_Ui* dialog=new DetInfoDialog_Ui(this, dlist);
    if (dialog->exec()) {
      list<DeviceEntry> entries;
      for(list<Pds::Src>::const_iterator iter=dialog->src_list().begin();
	  iter!=dialog->src_list().end(); iter++)
	entries.push_back(DeviceEntry(*iter));
      _expt.add_device(device, entries);
      update_device_list();
    }
    delete dialog;
  }
}  

void Devices_Ui::update_config_list()
{
  bool ok = disconnect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  _cfglist->clear();
  const Device* device(_device_c());
  if (device) {
    const list<TableEntry>& entries = device->table().entries();
    for(list<TableEntry>::const_iterator iter=entries.begin();
	iter!=entries.end(); iter++) {
      // exclude global configuration from direct editing
      if (iter->name() != string(GlobalCfg::name()))
	*new QListWidgetItem(iter->name().c_str(),_cfglist);
    }
  }
  if (ok) connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  update_component_list();
}

void Devices_Ui::update_component_list()
{
  bool ok_change = disconnect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
  bool ok_view   = disconnect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(view_component()));
  _cmplist->clear();
  _cmpaddlist->clear();
  _cmpremlist->clear();
    
  list<UTypeName> unassigned;
  for(unsigned i=0; i<PdsDefs::NumberOf; i++)
    unassigned.push_back(PdsDefs::utypeName(PdsDefs::ConfigType(i)));    

  const Device* device(_device_c());
  if (device) {
    const TableEntry* entry(0);
    QListWidgetItem* item = _cfglist->currentItem();
    if (item)
      entry = device->table().get_top_entry(string(qPrintable(item->text())));
    if (entry) {
      for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	  iter!=entry->entries().end(); iter++) {
	string label = iter->name() + " [" + iter->entry() + "]";
	*new QListWidgetItem(label.c_str(),_cmplist);
	unassigned.remove(UTypeName(iter->name()));
	_cmpremlist->addItem(UTypeName(iter->name()).c_str());
      }
    }
    //  List global entries here
    if ((entry = device->table().get_top_entry(string(GlobalCfg::name())))) {
      GlobalCfg::cache(_expt.path(),_device_c());
      for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	  iter!=entry->entries().end(); iter++) {
	string label = iter->name() + " [" + iter->entry() + "](G)";
	*new QListWidgetItem(label.c_str(),_cmplist);
	unassigned.remove(UTypeName(iter->name()));
	_cmpremlist->addItem(UTypeName(iter->name()).c_str());
      }
    }
  }

  for(list<UTypeName>::const_iterator iter=unassigned.begin(); 
      iter!= unassigned.end(); iter++)
    _cmpaddlist->addItem(iter->c_str());

  if (ok_change) connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
  if (ok_view  ) connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(view_component()));
}

bool Devices_Ui::validate_config_name(const string& name)
{
  if (name.size()==0) return false;
  Device* device(_device());
  if (!device) return false;
  list<TableEntry> tentries = device->table().entries();
  for(list<TableEntry>::const_iterator iter=tentries.begin();
      iter!=tentries.end(); iter++) 
    if (iter->name()==name)
      return false;
  return true;
}

void Devices_Ui::new_config()
{
  Device* device(_device());
  if (device) {
    string name(qPrintable(_cfgnewedit->text()));
    if (validate_config_name(name)) {
      device->table().new_top_entry(name);
      update_config_list();
    }
    else
      cout << "New config name \"" << name << "\" rejected" << endl;
  }
  else
    cout << "No device selected" << endl;
  _cfgnewedit->clear();
}

void Devices_Ui::copy_config()
{
  Device* device(_device());
  if (device) {
    string name(qPrintable(_cfgcpyedit->text()));
    if (validate_config_name(name)) {
      string src(qPrintable(_cfglist->currentItem()->text()));
      device->table().copy_top_entry(name,src);
      update_config_list();
    }
    else
      cout << "New config name \"" << name << "\" rejected" << endl;
  }
  else
    cout << "No device selected" << endl;
  _cfgcpyedit->clear();
}

void Devices_Ui::remove_config()
{
  Device* device(_device());
  if (device) {
    Table& table = device->table();
    string cfg(qPrintable(_cfglist->currentItem()->text()));
    table.remove_top_entry( *table.get_top_entry(cfg) );
    update_config_list();
  }
  else
    cout << "No device selected" << endl;
}

void Devices_Ui::change_component()
{
  string type(qPrintable(_cmplist->currentItem()->text()));
  add_component(QString(type.substr(0,type.find(' ')).c_str()));
}

void Devices_Ui::_current_component(string& utype,
                                    string& uname)
{
  if (_cmplist->currentItem()) {
    string type(qPrintable(_cmplist->currentItem()->text()));
    utype = type.substr(0,type.find(' '));
    UTypeName stype(utype);
    size_t fs  = type.find('[') + 1;
    size_t len = type.find(']') - fs; 
    uname = type.substr(fs,len);
  }
}

void Devices_Ui::view_component()
{
  string utype, uname;
  _current_component(utype,uname);

  const Pds::TypeId& type_id = *PdsDefs::typeId(UTypeName(utype));

  XtcEntry entry;
  entry.type_id = type_id;
  entry.name    = uname;

  DbClient& db = _expt.path();
  db.begin();
  int size = db.getXTC(entry);
  if (size <= 0) {
    QString msg = QString("Entry \'%1\' is either old configuration version or missing.\n  Try \'Browse Keys\' to read old version.").arg(uname.c_str());
    QMessageBox::warning(this, "Read file failed", msg);
    db.abort();
  }
  else {
    char* buff = new char[size];
    if (db.getXTC(entry,buff,size)==size) {
      db.commit();
      Dialog* d = new Dialog(_cmpaddlist, lookup(UTypeName(utype),_edit), 
                             entry.name.c_str(),
                             buff, size, _edit);
      if (d->exec()==QDialog::Accepted) {
        bool ok;
        QString file = QInputDialog::getText(this,"Configuration:","Name:",
                                             QLineEdit::Normal,d->name(),&ok);
        if (ok && !file.isEmpty()) {
          entry.name = qPrintable(file);
          db.begin();
          db.setXTC(entry, d->payload(), d->payload_size());
          db.commit();

          QListWidgetItem* item;
          item = _devlist->currentItem();
          if (!item) return;
          string det(qPrintable(item->text()));
          
          FileEntry entry(utype,qPrintable(d->name()));
          _expt.device(det)->table().set_entry(utype,entry);
        }
      }
      delete d;
    }
    delete[] buff;
  }
}

void Devices_Ui::add_component(const QString& type)
{
  string strtype(qPrintable(type));
  UTypeName stype(strtype);
  const Pds::TypeId& type_id = *PdsDefs::typeId(stype);

  QListWidgetItem* item;
  item = _devlist->currentItem();
  if (!item) return;
  string det(qPrintable(item->text()));

  string cfg;
  if (GlobalCfg::contains(stype)) {  // create the global alias
    cfg = string(GlobalCfg::name());
    if (_expt.device(det)->table().get_top_entry(cfg) == 0)
      _expt.device(det)->table().new_top_entry(cfg);
  }
  else {
    item = _cfglist->currentItem();
    if (!item) return;
    cfg = string(qPrintable(item->text()));
  }

  QStringList choices;
  list<XtcEntry> entries = _expt.path().getXTC(type_id.value());
  for(list<XtcEntry>::const_iterator it=entries.begin();
      it!=entries.end(); it++)
    choices << it->name.c_str();

  const string create_str("-CREATE-");
  const string import_str("-IMPORT-");
  choices << create_str.c_str();
  choices << import_str.c_str();

  int current;
  { string utype, uname;
    _current_component(utype,uname);
    current = choices.indexOf(QString(uname.c_str())); 
    if (current<0) current=0; }

  bool ok;
  QString choice = QInputDialog::getItem(_devlist,
           "Component Data",
           "Select File",
           choices, current, 0, &ok);
  if (ok) {
    Dialog*        d = 0;
    char*    payload = 0;
    int   payload_sz = 0;
    DbClient& db = _expt.path();
    XtcEntry x;
    x.type_id = type_id;

    string schoice(qPrintable(choice));
    if (schoice==create_str) {
      d = new Dialog(_cmpaddlist, lookup(stype,true), "example.xtc", true);
    }
    else if (schoice==import_str) {
      QString file = QFileDialog::getOpenFileName(_devlist,
              "Import Data File",
              "","");
      schoice = basename(const_cast<char*>(qPrintable(file)));
      if (!file.isEmpty()) {
	string sfile(qPrintable(file));
        struct stat64 s;
        if (stat64(sfile.c_str(),&s))
          perror("stat64 xtc file");
        else {
          payload_sz = s.st_size;
          payload = new char[payload_sz];
          FILE* f = fopen(sfile.c_str(),"r");
          if (fread(payload, 1, payload_sz, f) != size_t(payload_sz)) {
            perror("fread xtc file");
            delete[] payload;
            payload = 0;
          }
          d = new Dialog(_cmpaddlist, lookup(stype,true), 
                         schoice.c_str(),
                         payload, payload_sz, true);
        }
      }
    }
    else {
      x.name    = schoice;
      payload_sz = db.getXTC(x);
      if (payload_sz <= 0) {
        perror("Error fetching xtc");
        payload_sz = 0;
      }
      else {
        payload = new char[payload_sz];
        db.getXTC(x,payload,payload_sz);
        d = new Dialog(_cmpaddlist, lookup(stype,true), 
                       schoice.c_str(),
                       payload, payload_sz, true);
      }
    }

    if (d) {
      if (d->exec()==QDialog::Accepted) {
        bool ok;
        QString file = QInputDialog::getText(this,"Configuration:","Name:",
                                             QLineEdit::Normal,d->name(),&ok);
        if (ok && !file.isEmpty()) {
          x.name = qPrintable(file);
          db.begin();
          db.setXTC(x, d->payload(), d->payload_size());
          db.commit();
          
          FileEntry entry(stype,qPrintable(d->name()));
          _expt.device(det)->table().set_entry(cfg,entry);
        }
      }
      delete d;
    }

    if (payload)
      delete[] payload;
  }
  update_component_list();
}

void Devices_Ui::remove_component(const QString& type)
{
  string strtype(qPrintable(type));
  UTypeName stype(strtype);

  QListWidgetItem* item;
  item = _devlist->currentItem();
  if (!item) return;
  string det(qPrintable(item->text()));

  string cfg;
  if (GlobalCfg::contains(stype)) {  // create the global alias
    cfg = string(GlobalCfg::name());
    if (_expt.device(det)->table().get_top_entry(cfg) == 0)
      _expt.device(det)->table().new_top_entry(cfg);
  }
  else {
    item = _cfglist->currentItem();
    if (!item) return;
    cfg = string(qPrintable(item->text()));
  }

  FileEntry entry(stype,"");
  _expt.device(det)->table().clear_entry(cfg, entry);

  update_component_list();
}

void Devices_Ui::expert_mode() { _expert_mode=true; }
void Devices_Ui::user_mode  () { _expert_mode=false; }

Serializer& Devices_Ui::lookup(const UTypeName& stype, bool edit)
{ 
  Parameter::allowEdit(edit);
  Serializer& s = _expert_mode ?
    *_xdict.lookup(*PdsDefs::typeId(stype)) :
    *_dict .lookup(*PdsDefs::typeId(stype));
  //  s.setPath(_expt.path());
  return s;
}    

