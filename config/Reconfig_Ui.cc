#include "pdsapp/config/Reconfig_Ui.hh"

#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Device.hh"
#include "pds/config/PdsDefs.hh"
#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/Restore_Ui.hh"

#include "pds/config/DbClient.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>

#include <list>
using std::list;

#include <stdio.h>

using namespace Pds_ConfigDb;

enum { EditMode, RestoreMode };

Reconfig_Ui::Reconfig_Ui(QWidget* parent,
			 Experiment& expt) :
  QDialog(parent),
  _expt    (expt),
  _entry   (0),
  _expert_mode(false)
{
  setWindowTitle("Reconfigure");
  setAttribute(::Qt::WA_DeleteOnClose, false);
  setModal(false);

  QRadioButton* editB = new QRadioButton("Edit");
  QRadioButton* restB = new QRadioButton("Restore");

  _modeG  = new QButtonGroup;
  _modeG->addButton(editB,EditMode);
  _modeG->addButton(restB,RestoreMode);

  _applyB = new QPushButton("Apply");
  QPushButton* closeB = new QPushButton("Close");

  QVBoxLayout* l = new QVBoxLayout;
  { QMenuBar* menu_bar = new QMenuBar;
    QMenu* mode_menu = new QMenu("Options");
    mode_menu->addAction("Expert mode", this, SLOT(expert_mode()));
    mode_menu->addAction("User mode", this, SLOT(user_mode()));
    menu_bar->addMenu(mode_menu);
    l->addWidget(menu_bar); }
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
    layout->addWidget(editB);
    layout->addWidget(restB);
    layout->addStretch();
    l->addLayout(layout); }
  { QHBoxLayout* layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(_applyB);
    layout->addWidget(closeB);
    layout->addStretch();
    l->addLayout(layout); }
  setLayout(l);

  connect(_devlist, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(update_component_list()));
  connect(_cmplist, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(change_component()));
  connect(_applyB , SIGNAL(clicked()), this, SLOT(apply()));
  connect(closeB  , SIGNAL(clicked()), this, SLOT(hide()));
  connect(_applyB , SIGNAL(clicked()), this, SLOT(reset()));
  connect(closeB  , SIGNAL(clicked()), this, SLOT(reset()));

  reset();
  update_device_list();
}

void Reconfig_Ui::reset()
{
  _modeG->button(EditMode)->setChecked(true);
}

void Reconfig_Ui::enable(bool v)
{
  if (v) {
    _applyB->setPalette(QPalette());
    _applyB->setFont(QFont());
    _applyB->setText("Apply");
    _applyB->setEnabled(true);
  }
  else {
    _applyB->setFont(QFont("Helvetica", 8));
    _applyB->setText("Apply\n( Inactive during\n  remote control )");
    _applyB->setEnabled(false);
  }
}

Device* Reconfig_Ui::_device() const 
{ 
  QListWidgetItem* item = _devlist->currentItem();
  return item ? _expt.device(string(qPrintable(item->text()))) : 0;
}

const TableEntry* Reconfig_Ui::_device_entry() const
{
  Device* device = _device();
  if (device) {
    for(list<FileEntry>::const_iterator iter=_entry->entries().begin();
	iter!=_entry->entries().end(); iter++)
      if (iter->name() == device->name())
	return device->table().get_top_entry(iter->entry());
  }
  return 0;
}

void Reconfig_Ui::apply()
{
  if (_entry) {
    _expt.update_keys();
    _expt.write();
    emit changed();
  }
}

void Reconfig_Ui::set_run_type(const QString& runType)
{
  _entry = _expt.table().get_top_entry(std::string(qPrintable(runType)));
  update_device_list();
}

void Reconfig_Ui::update_device_list()
{
  bool ok = disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  _devlist->clear();
  if (_entry) {
    for(list<FileEntry>::const_iterator iter=_entry->entries().begin();
	iter!=_entry->entries().end(); iter++) {
      *new QListWidgetItem(QString(iter->name().c_str()),_devlist);
    }
  }
  if (ok) connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  update_component_list();
}

void Reconfig_Ui::update_component_list()
{
  bool ok_change = disconnect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
  _cmplist->clear();

  QListWidgetItem* item = _devlist->currentItem();
  if (item) {
    const TableEntry* entry = _device_entry();
    if (entry)
      for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	  iter!=entry->entries().end(); iter++)
	*new QListWidgetItem(QString(iter->name().c_str()),_cmplist);
    //  List global entries here
    if ((entry = _device()->table().get_top_entry(string(GlobalCfg::name())))) {
      GlobalCfg::cache(_expt.path(),_device());
      for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	  iter!=entry->entries().end(); iter++)
	*new QListWidgetItem(QString(iter->name().c_str()),_cmplist);
    }
  }

  if (ok_change) connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
}

void Reconfig_Ui::change_component()
{
  DbClient& db = _expt.path();

  std::string t(qPrintable(_cmplist->currentItem()->text()));
  UTypeName stype(t);
  QString qchoice;
  char*    payload = 0;
  unsigned payload_sz = 0;
  const TableEntry* entry;
  if (GlobalCfg::contains(stype)) {
    entry = _device()->table().get_top_entry(string(GlobalCfg::name()));
  }
  else {
    entry = _device_entry();
  }
  if (entry) {
    for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	iter!=entry->entries().end(); iter++)
      if (iter->name()==t) {
	qchoice = iter->entry().c_str();

	if (_modeG->checkedId()==EditMode) {

	  XtcEntry x;
	  x.type_id = *PdsDefs::typeId(stype);
	  x.name    = iter->entry();
	  
	  db.begin();
	  if ((payload_sz = db.getXTC(x))<=0)
	    db.abort();
	  else {
	    db.commit();

	    payload = new char[payload_sz];
	    db.begin();
	    db.getXTC(x,payload,payload_sz);
	    db.commit();

	    //  edit the contents of the file	
	    Parameter::allowEdit(true);
	    Dialog* d = new Dialog(this, lookup(stype), qchoice,
				   payload, payload_sz, true);
	    if (d->exec()==QDialog::Accepted) {
          
	      x.name = qPrintable(d->name());
	      db.begin();
	      db.setXTC(x, d->payload(), d->payload_size());
	      db.commit();
          
	      if (GlobalCfg::contains(stype)) {
		char* b = new char[d->payload_size()];
		memcpy(b, d->payload(), d->payload_size());
		GlobalCfg::cache(stype,b);
	      }
	    }
	    delete d;
	    delete[] payload;
	  }
	}
	else { // RestoreMode
	  Restore_Ui* d = new Restore_Ui(this, db, 
					 *_device(), 
					 *PdsDefs::typeId(stype), 
					 iter->entry());
	  d->exec();
	  delete d;
	}
	break;
      }
  }
  if (qchoice.isEmpty())
    printf("Error looking up %s\n", t.c_str());
}

void Reconfig_Ui::expert_mode() { _expert_mode=true; }
void Reconfig_Ui::user_mode  () { _expert_mode=false; }

Serializer& Reconfig_Ui::lookup(const UTypeName& stype)
{ 
  Serializer& s = _expert_mode ? 
    *_xdict.lookup(*PdsDefs::typeId(stype)) :
    *_dict .lookup(*PdsDefs::typeId(stype));
  //  s.setPath(_expt.path());
  return s;
}    
