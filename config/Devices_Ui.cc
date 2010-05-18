#include "pdsapp/config/Devices_Ui.hh"

#include "pdsapp/config/DetInfoDialog_Ui.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Device.hh"
#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/GlobalCfg.hh"

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

#include <list>
#include <string>
#include <iostream>
#include <sys/stat.h>

using std::list;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include <sys/stat.h>
#include <libgen.h>

using namespace Pds_ConfigDb;

Devices_Ui::Devices_Ui(QWidget* parent,
           Experiment& expt,
           bool edit) :
  QGroupBox("Devices Configuration", parent),
  _expt    (expt)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Device", this));
    layout1->addWidget(_devlist = new QListWidget(this));
    layout1->addWidget(_deveditbutton = new QPushButton("Edit", this));
    { QHBoxLayout* layout1a = new QHBoxLayout;
      layout1a->addWidget(_devnewedit   = new QLineEdit(this));
      layout1a->addWidget(_devnewbutton = new QPushButton("New", this));
      layout1->addLayout(layout1a); }
    layout->addLayout(layout1); }
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Config Name", this));
    layout1->addWidget(_cfglist = new QListWidget(this));
    { QHBoxLayout* layout1a = new QHBoxLayout;
      layout1a->addWidget(_cfgnewedit   = new QLineEdit(this));
      layout1a->addWidget(_cfgnewbutton = new QPushButton("New", this));
      layout1->addLayout(layout1a); }
    { QHBoxLayout* layout1a = new QHBoxLayout;
      layout1a->addWidget(_cfgcpyedit   = new QLineEdit(this));
      layout1a->addWidget(_cfgcpybutton = new QPushButton("Copy To", this));
      layout1->addLayout(layout1a); }
    layout->addLayout(layout1); }
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Component", this));
    layout1->addWidget(_cmplist = new QListWidget(this));
    { QHBoxLayout* layout1a = new QHBoxLayout;
      QLabel* label = new QLabel("Add", this);
      label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter );
      layout1a->addWidget(label);
      layout1a->addWidget(_cmpcfglist = new QComboBox(this));
      layout1->addLayout(layout1a); }
    layout->addLayout(layout1); }
  setLayout(layout);

  if (edit) {
    connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_config_list()));
    connect(_deveditbutton, SIGNAL(clicked()), this, SLOT(edit_device()));
    connect(_devnewbutton, SIGNAL(clicked()), this, SLOT(new_device()));
    connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
    connect(_cfgnewbutton, SIGNAL(clicked()), this, SLOT(new_config()));
    connect(_cfgcpybutton, SIGNAL(clicked()), this, SLOT(copy_config()));
    connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
    connect(_cmpcfglist, SIGNAL(activated(const QString&)), this, SLOT(add_component(const QString&)));
  }
  else {
    connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_config_list()));
    _deveditbutton->setEnabled(false);
    _devnewbutton ->setEnabled(false);
    connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
    _cfgnewbutton ->setEnabled(false);
    _cfgcpybutton ->setEnabled(false);
    connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(view_component()));
  }
  update_device_list();
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
  Device* device(_device());
  if (device) {
    const list<TableEntry>& entries = device->table().entries();
    for(list<TableEntry>::const_iterator iter=entries.begin();
	iter!=entries.end(); iter++)
      // exclude global configuration from direct editing
      if (iter->name() != string(GlobalCfg::name()))
	*new QListWidgetItem(iter->name().c_str(),_cfglist);
  }
  if (ok) connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  update_component_list();
}

void Devices_Ui::update_component_list()
{
  bool ok_change = disconnect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
  bool ok_view   = disconnect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(view_component()));
  _cmplist->clear();
  _cmpcfglist->clear();
    
  list<UTypeName> unassigned;
  for(unsigned i=0; i<PdsDefs::NumberOf; i++)
    unassigned.push_back(PdsDefs::utypeName(PdsDefs::ConfigType(i)));    

  Device* device(_device());
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
      }
    }
    //  List global entries here
    if ((entry = device->table().get_top_entry(string(GlobalCfg::name())))) {
      GlobalCfg::cache(_expt.path(),_device());
      for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	  iter!=entry->entries().end(); iter++) {
	string label = iter->name() + " [" + iter->entry() + "](G)";
	*new QListWidgetItem(label.c_str(),_cmplist);
	unassigned.remove(UTypeName(iter->name()));
      }
    }
  }

  for(list<UTypeName>::const_iterator iter=unassigned.begin(); 
      iter!= unassigned.end(); iter++)
    _cmpcfglist->addItem(iter->c_str());

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

void Devices_Ui::change_component()
{
  string type(qPrintable(_cmplist->currentItem()->text()));
  add_component(QString(type.substr(0,type.find(' ')).c_str()));
}

void Devices_Ui::view_component()
{
  string type(qPrintable(_cmplist->currentItem()->text()));
  QString qtype(type.substr(0,type.find(' ')).c_str());
  string strtype(qPrintable(qtype));
  UTypeName stype(strtype);
  size_t fs  = type.find('[') + 1;
  size_t len = type.find(']') - fs; 
  QString qname(type.substr(fs,len).c_str());

  string path(_expt.path().data_path("",stype));
  QString qpath(path.c_str());
  QString qfile = qpath + "/" + qname;

  struct stat s;
  if (stat(qPrintable(qfile),&s)) {
    QString msg = QString("File \'%1\' is either old configuration version or missing.\n  Try \'Browse Keys\' to read old version.").arg(qname);
    QMessageBox::warning(this, "Read file failed", msg);
  }
  else {
    Dialog* d = new Dialog(_cmpcfglist, lookup(stype), qpath, qpath, qfile);
    d->exec();
    delete d;
  }
}

void Devices_Ui::add_component(const QString& type)
{
  string strtype(qPrintable(type));
  UTypeName stype(strtype);

  QListWidgetItem* item;
  item = _devlist->currentItem();
  if (!item) return;
  string det(qPrintable(item->text()));

  string cfg;
  if (GlobalCfg::contains(*PdsDefs::typeId(stype))) {  // create the global alias
    cfg = string(GlobalCfg::name());
    if (_expt.device(det)->table().get_top_entry(cfg) == 0)
      _expt.device(det)->table().new_top_entry(cfg);
  }
  else {
    item = _cfglist->currentItem();
    if (!item) return;
    cfg = string(qPrintable(item->text()));
  }

  list<string> xtc_files = _expt.path().xtc_files(det,stype);
  QStringList choices;
  for(list<string>::const_iterator iter=xtc_files.begin(); iter!=xtc_files.end(); iter++)
    choices << iter->c_str();
  const string create_str("-CREATE-");
  const string import_str("-IMPORT-");
  choices << create_str.c_str();
  choices << import_str.c_str();
  bool ok;
  QString choice = QInputDialog::getItem(_devlist,
           "Component Data",
           "Select File",
           choices, 0, 0, &ok);
  if (ok) {
    string schoice(qPrintable(choice));
    if (schoice==import_str) {
      QString file = QFileDialog::getOpenFileName(_devlist,
              "Import Data File",
              "","");
      if (!file.isEmpty()) {
	string sfile(qPrintable(file));
	_expt.import_data(det,stype,sfile,"");
	FileEntry entry(stype,basename(const_cast<char*>(sfile.c_str())));
	_expt.device(det)->table().set_entry(cfg,entry);
      }
    }
    else if (schoice==create_str) {
      string path(_expt.path().data_path("",stype));
      QString qpath(path.c_str());

      Dialog* d = new Dialog(_cmpcfglist, lookup(stype), qpath, qpath);
      d->exec();
      QString file(d->file());
      delete d;
      if (!file.isEmpty()) {
	string sfile(qPrintable(file));
	FileEntry entry(stype,sfile);
	_expt.device(det)->table().set_entry(cfg,entry);
      }
    }
    else {
      string path(_expt.path().data_path("",stype));
      QString qpath(path.c_str());
      QString qchoice = qpath + "/" + choice;

      //      Parameter::allowEdit(false);
      Dialog* d = new Dialog(_cmpcfglist, lookup(stype), qpath, qpath, qchoice);
      d->exec();
      delete d;
      //      Parameter::allowEdit(true);

      FileEntry entry(stype,schoice);
      _expt.device(det)->table().set_entry(cfg,entry);
    }
  }
  update_component_list();
}
        
Serializer& Devices_Ui::lookup(const UTypeName& stype)
{ 
  Serializer& s = *_dict.lookup(*PdsDefs::typeId(stype));
  s.setPath(_expt.path());
  return s;
}    
