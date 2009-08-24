#include "pdsapp/control/ConfigSelect.hh"
#include "pdsapp/config/Experiment.hh"
#include "pds/management/PartitionControl.hh"

#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

using namespace Pds;
using Pds_ConfigDb::Experiment;
using Pds_ConfigDb::Table;
using Pds_ConfigDb::TableEntry;

ConfigSelect::ConfigSelect(QWidget*          parent,
			   PartitionControl& control,
			   const char*       db_path) :
  QGroupBox("Configuration",parent),
  _pcontrol(control),
  _expt    (Pds_ConfigDb::Path(db_path))
{
  _expt.read();

  QPushButton* bView;
  QPushButton* bUpdate;

  QHBoxLayout* layout = new QHBoxLayout;
  { QGroupBox* b = new QGroupBox("Select", this);
    QGridLayout* layout1 = new QGridLayout;
    layout1->addWidget(new QLabel("Type",this),0,0,1,1,Qt::AlignHCenter);
    layout1->addWidget(_runType = new QComboBox(this),0,1,1,1);
    layout1->addWidget(new QLabel("Key",this) ,1,0,1,1,Qt::AlignHCenter);
    layout1->addWidget(_runKey  = new QLineEdit(this),1,1,1,1);
    b->setLayout(layout1);
    layout->addWidget(b); }
  { QGroupBox* b = new QGroupBox("Database", this);
    QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(bView   = new QPushButton("View"  ,b));
    layout1->addWidget(bUpdate = new QPushButton("Update",b));
    b->setLayout(layout1);
    layout->addWidget(b); }
  setLayout(layout);

  _runKey->setValidator(new QIntValidator(_runKey));

  bView->setEnabled(false);
  connect(bUpdate,  SIGNAL(clicked()),
	  this,     SLOT(update_run_types()));
  connect(_runType, SIGNAL(activated(const QString&)), 
	  this, SLOT(set_run_type(const QString&)));
  connect(_runKey , SIGNAL(textEdited(const QString&)),
	  this, SLOT(set_run_key(const QString&)));

  update_run_types();
}

ConfigSelect::~ConfigSelect() 
{
}

unsigned ConfigSelect::run_key() const
{
  return _runKey->text().toInt();
}

void ConfigSelect::set_run_type(const QString& run_type)
{
  const list<TableEntry>& l = _expt.table().entries();
  for(list<TableEntry>::const_iterator iter = l.begin(); iter != l.end(); ++iter)
    if (run_type == iter->name().c_str()) {
      char buff[32];
      sprintf(buff,"%lu",strtoul(iter->key().c_str(),NULL,16));
      _runKey->setText(buff);
      set_run_key(buff);
    }
}

void ConfigSelect::set_run_key(const QString&)
{
  int key = run_key();
  _pcontrol.set_transition_env(TransitionId::Configure, key);
  printf("Set run key to %d\n",key);
}

void ConfigSelect::update_run_types()
{
  _runType->clear();

  const list<TableEntry>& l = _expt.table().entries();
  for(list<TableEntry>::const_iterator iter = l.begin(); iter != l.end(); ++iter)
    _runType->addItem(iter->name().c_str());

  set_run_type(l.begin()->name().c_str());
}

void ConfigSelect::allocated()
{
  _runKey ->setEnabled(false);
  _runType->setEnabled(false);
}

void ConfigSelect::deallocated()
{
  _runKey ->setEnabled(true);
  _runType->setEnabled(true);
}
