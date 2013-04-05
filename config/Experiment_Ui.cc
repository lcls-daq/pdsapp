#include "pdsapp/config/Experiment_Ui.hh"

#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/GlobalCfg.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QInputDialog>

#include <iostream>
#include <list>
using std::cout;
using std::endl;
using std::list;

using namespace Pds_ConfigDb;

Experiment_Ui::Experiment_Ui(QWidget* parent,
			     Experiment& expt,
			     bool edit) :
  QGroupBox("Experiment Configuration", parent),
  _expt    (expt)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  QPushButton* cfgrembutton = new QPushButton("Remove");

  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Config Name", this));
    layout1->addWidget(_cfglist = new QListWidget(this));
    { QGridLayout* layout1a = new QGridLayout;
      layout1a->addWidget(_cfgnewedit = new QLineEdit(this), 0, 0 );
      layout1a->addWidget(_cfgnewbutton = new QPushButton("New", this), 0, 1 );
      layout1a->addWidget(_cfgcopyedit   = new QLineEdit(this), 1, 0 );
      layout1a->addWidget(_cfgcopybutton = new QPushButton("Copy To", this), 1, 1);
      layout1a->addWidget(cfgrembutton, 0, 2, 2, 1);
      layout1->addLayout(layout1a); }
    layout->addLayout(layout1); }

  { QVBoxLayout* layout2 = new QVBoxLayout;
    layout2->addWidget(new QLabel("Device Configuration", this));
    layout2->addWidget(_devlist = new QListWidget(this));
    { QHBoxLayout* layout2a = new QHBoxLayout;
      QLabel* label = new QLabel("Add", this);
      label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
      layout2a->addWidget(label);
      layout2a->addWidget(_adddevlist = new QComboBox(this)); 
      layout2->addLayout(layout2a); }
   { QHBoxLayout* layout2a = new QHBoxLayout;
      QLabel* label = new QLabel("Remove", this);
      label->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
      layout2a->addWidget(label);
      layout2a->addWidget(_remdevlist = new QComboBox(this)); 
      layout2->addLayout(layout2a); }
    layout->addLayout(layout2);
  }
  setLayout(layout);

  connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
  if (edit) {
    connect(_cfgnewbutton, SIGNAL(clicked()), this, SLOT(new_config()));
    connect( cfgrembutton, SIGNAL(clicked()), this, SLOT(remove_config()));
    connect(_cfgcopybutton, SIGNAL(clicked()), this, SLOT(copy_config()));
    connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(device_changed()));
    connect(_adddevlist, SIGNAL(activated(const QString&)), this, SLOT(add_device(const QString&)));
    connect(_remdevlist, SIGNAL(activated(const QString&)), this, SLOT(remove_device(const QString&)));
  }
  else {
    _cfgnewbutton ->setEnabled(false);
    _cfgcopybutton->setEnabled(false);
    _devlist      ->setEnabled(false);
    _adddevlist   ->setEnabled(false);
    _remdevlist   ->setEnabled(false);
  }
  update_config_list();
}

Experiment_Ui::~Experiment_Ui()
{
}

void Experiment_Ui::db_update()
{
  int i = _cfglist->currentRow();
  update_config_list();
  if (i>=0)
    _cfglist->setCurrentRow(i);
}

bool Experiment_Ui::validate_config_name(const string& name)
{
  if (name.size()==0) return false;
  list<string> names(_expt.table().get_top_names());
  for(list<string>::const_iterator iter=names.begin(); iter!=names.end(); iter++)
    if (*iter==name) return false;
  return true;
}

void Experiment_Ui::update_device_list()
{
  string name(qPrintable(_cfglist->currentItem()->text()));
  bool ok1 = disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(device_changed()));
  bool ok2 = disconnect(_adddevlist, SIGNAL(activated(const QString&)), this, SLOT(add_device(const QString&)));
  bool ok3 = disconnect(_remdevlist, SIGNAL(activated(const QString&)), this, SLOT(remove_device(const QString&)));
  _devlist->clear();
  _adddevlist->clear();
  _remdevlist->clear();

  list<string> unassigned;
  for(list<Device>::iterator iter = _expt.devices().begin(); iter != _expt.devices().end();
      iter++)
    unassigned.push_back(iter->name());

  const TableEntry* entry = _expt.table().get_top_entry(name);
  for(list<FileEntry>::const_iterator iter=entry->entries().begin();
      iter!=entry->entries().end(); iter++) {
    string entry = iter->name() + " [" + iter->entry() + "]";
    *new QListWidgetItem(entry.c_str(),_devlist);
    unassigned.remove(iter->name());
    _remdevlist->addItem(iter->name().c_str());
  }

  for(list<string>::iterator iter = unassigned.begin(); iter != unassigned.end();
      iter++)
    _adddevlist->addItem(iter->c_str());

  if (ok1) connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(device_changed()));
  if (ok2) connect(_adddevlist, SIGNAL(activated(const QString&)), this, SLOT(add_device(const QString&)));
  if (ok3) connect(_remdevlist, SIGNAL(activated(const QString&)), this, SLOT(remove_device(const QString&)));
}

void Experiment_Ui::new_config()
{
  string name(qPrintable(_cfgnewedit->text()));
  if (validate_config_name(name)) {
    _expt.table().new_top_entry(name);
    update_config_list();
  }
  else
    cout << "New config name \"" << name << "\" rejected" << endl;

  _cfgnewedit->clear();
}

void Experiment_Ui::remove_config()
{
  if (_cfglist->currentItem()) {
    string name(qPrintable(_cfglist->currentItem()->text()));
    _expt.table().remove_top_entry( *_expt.table().get_top_entry(name) );
    update_config_list();
  }
}

void Experiment_Ui::copy_config()
{
  string name(qPrintable(_cfgcopyedit->text()));
  if (validate_config_name(name)) {
    string src(qPrintable(_cfglist->currentItem()->text()));
    _expt.table().copy_top_entry(name,src);
    update_config_list();
  }
  else
    cout << "New config name \"" << name << "\" rejected" << endl;

  _cfgcopyedit->clear();
}

void Experiment_Ui::device_changed()
{
  string sel(qPrintable(_devlist->currentItem()->text()));
  string device(sel.substr(0,sel.find(' ')));
  change_device(device);
}

void Experiment_Ui::add_device(const QString& name)
{
  string device(qPrintable(name));
  change_device(device);
}

void Experiment_Ui::remove_device(const QString& name)
{
  string cfg(qPrintable(_cfglist->currentItem()->text()));
  string dev(qPrintable(name));
  TableEntry* entry = const_cast<TableEntry*>(_expt.table().get_top_entry(cfg));
  if (entry) {
    const list<FileEntry>& entries = entry->entries();
    for(list<FileEntry>::const_iterator iter = entries.begin();
	iter != entries.end(); iter++)
      if (iter->name() == dev) {
	entry->remove(*iter);
	update_device_list();
	break;
      }
  }
}

void Experiment_Ui::change_device(const string& device)
{
  QStringList choices;
  const list<TableEntry>& te = _expt.device(device)->table().entries();
  for(list<TableEntry>::const_iterator iter=te.begin(); iter!=te.end(); iter++)
    if (iter->name() != string(GlobalCfg::name()))
      choices << iter->name().c_str();
  bool ok;
  QString choice = QInputDialog::getItem(_devlist,
					 (device + " Configuration").c_str(),
					 "Select Config",
					 choices, 0, 0, &ok);
  if (ok) {
    string cfg(qPrintable(_cfglist->currentItem()->text()));
    FileEntry fe(device,string(qPrintable(choice)));
    _expt.table().set_entry(cfg,fe);
    update_device_list();
  }
}

void Experiment_Ui::update_config_list()
{
  bool ok = disconnect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
  _cfglist->clear();
  list<TableEntry>& l = _expt.table().entries();
  for(list<TableEntry>::const_iterator iter = l.begin(); iter != l.end(); ++iter) {
    *new QListWidgetItem(iter->name().c_str(), _cfglist);
  }
  if (ok) connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
}


