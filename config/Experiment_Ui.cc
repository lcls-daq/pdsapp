#include "pdsapp/config/Experiment_Ui.hh"

#include "pdsapp/config/Experiment.hh"

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
			     Experiment& expt) :
  QGroupBox("Experiment Configuration", parent),
  _expt    (expt)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  { QVBoxLayout* layout1 = new QVBoxLayout;
  layout1->addWidget(new QLabel("Config Name", this));
  layout1->addWidget(_cfglist = new QListWidget(this));
  { QHBoxLayout* layout1a = new QHBoxLayout;
  layout1a->addWidget(_cfgnewedit   = new QLineEdit(this));
  layout1a->addWidget(_cfgnewbutton = new QPushButton("New", this));
  layout1->addLayout(layout1a); }
  { QHBoxLayout* layout1b = new QHBoxLayout;
  layout1b->addWidget(_cfgcopyedit   = new QLineEdit(this));
  layout1b->addWidget(_cfgcopybutton = new QPushButton("Copy To", this));
  layout1->addLayout(layout1b); }
  layout->addLayout(layout1); }

  { QVBoxLayout* layout2 = new QVBoxLayout;
  layout2->addWidget(new QLabel("Device Configuration", this));
  layout2->addWidget(_devlist = new QListWidget(this));
  { QHBoxLayout* layout2a = new QHBoxLayout;
  QLabel* label = new QLabel("Add", this);
  label->setAlignment(Qt::AlignRight | Qt::AlignTrailing);
  layout2a->addWidget(label);
  layout2a->addWidget(_devcfglist = new QComboBox(this)); 
  layout2->addLayout(layout2a); }
  layout->addLayout(layout2);
  }
  setLayout(layout);

  connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
  connect(_cfgnewbutton, SIGNAL(clicked()), this, SLOT(new_config()));
  connect(_cfgcopybutton, SIGNAL(clicked()), this, SLOT(copy_config()));
  connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(device_changed()));
  connect(_devcfglist, SIGNAL(activated(const QString&)), this, SLOT(add_device(const QString&)));
  
  update_config_list();
}

Experiment_Ui::~Experiment_Ui()
{
}

void Experiment_Ui::db_update()
{
  update_config_list();
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
  disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(device_changed()));
  _devlist->clear();
  const TableEntry* entry = _expt.table().get_top_entry(name);
  for(list<FileEntry>::const_iterator iter=entry->entries().begin();
      iter!=entry->entries().end(); iter++) {
    string entry = iter->name() + " [" + iter->entry() + "]";
    *new QListWidgetItem(entry.c_str(),_devlist);
  }
  connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(device_changed()));
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

void Experiment_Ui::change_device(const string& device)
{
  QStringList choices;
  const list<TableEntry>& te = _expt.device(device)->table().entries();
  for(list<TableEntry>::const_iterator iter=te.begin(); iter!=te.end(); iter++)
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
  disconnect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
  _cfglist->clear();
  list<TableEntry>& l = _expt.table().entries();
  for(list<TableEntry>::const_iterator iter = l.begin(); iter != l.end(); ++iter)
    *new QListWidgetItem(iter->name().c_str(), _cfglist);
  connect(_cfglist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
}








