#include "pdsapp/config/Reconfig_Ui.hh"

#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Device.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

#include <list>

using std::list;

using namespace Pds_ConfigDb;

Reconfig_Ui::Reconfig_Ui(QWidget* parent,
			 Experiment& expt) :
  QDialog(parent),
  _expt    (expt),
  _entry   (0)
{
  setWindowTitle("Reconfigure");
  setAttribute(::Qt::WA_DeleteOnClose, false);
  setModal(false);
  Parameter::allowEdit(true);

  QPushButton* applyB = new QPushButton("Apply");
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
    layout->addWidget(applyB);
    layout->addWidget(closeB);
    layout->addStretch();
    l->addLayout(layout); }
  setLayout(l);

  connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_component_list()));
  connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
  connect(applyB  , SIGNAL(clicked()), this, SLOT(apply()));
  connect(closeB  , SIGNAL(clicked()), this, SLOT(apply()));
  connect(closeB  , SIGNAL(clicked()), this, SLOT(hide()));

  update_device_list();
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
    _expt.update_key(*_entry);
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
  }

  if (ok_change) connect(_cmplist, SIGNAL(itemSelectionChanged()), this, SLOT(change_component()));
}

void Reconfig_Ui::change_component()
{
  const TableEntry* entry = _device_entry();
  if (entry) {
    std::string t(qPrintable(_cmplist->currentItem()->text()));
    for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	iter!=entry->entries().end(); iter++)
      if (iter->name()==t) {
	UTypeName stype(iter->name());
	string path = _expt.path().data_path("",stype);

	QString qchoice = QString("%1/%2").arg(path.c_str()).arg(iter->entry().c_str());

	//  edit the contents of the file	
	Dialog* d = new Dialog(this, lookup(stype), qchoice);
	d->exec();
	delete d;

	break;
      }
  }
}
        
Serializer& Reconfig_Ui::lookup(const UTypeName& stype)
{ 
  return *_dict.lookup(*PdsDefs::typeId(stype));
}    
