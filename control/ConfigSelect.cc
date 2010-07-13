#include "pdsapp/control/ConfigSelect.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Reconfig_Ui.hh"
#include "pdsapp/config/ControlScan.hh"
#include "pds/management/PartitionControl.hh"

#include <QtGui/QComboBox>
#include <QtGui/QLabel>
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

  _reconfig = new Pds_ConfigDb::Reconfig_Ui(this, _expt);
  _scan     = new Pds_ConfigDb::ControlScan(this, _expt);

  QPushButton* bEdit = new QPushButton("Edit");
  QPushButton* bScan = new QPushButton("Scan");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(new QLabel("Type"));
    layout1->addWidget(_runType = new QComboBox);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(bEdit);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(bScan);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(bEdit   , SIGNAL(clicked()),                 _reconfig, SLOT(show()));
  connect(bScan   , SIGNAL(clicked(bool)),	       this, SLOT(enable_scan(bool)));
  connect(_runType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(set_run_type(const QString&)));
  connect(_reconfig,SIGNAL(changed()),                 this, SLOT(update()));
  connect(_scan    ,SIGNAL(created(int)),              this, SLOT(run_scan(int)));

  read_db();
  set_run_type(_runType->currentText());

  bScan->setCheckable(true);
  bScan->setChecked  (false);
}

ConfigSelect::~ConfigSelect() 
{
}

void ConfigSelect::set_run_type(const QString& run_type)
{
  _reconfig->set_run_type(run_type);
  _scan    ->set_run_type(run_type);

  const TableEntry* e = _expt.table().get_top_entry(qPrintable(run_type));
  if (e) {
    _run_key = strtoul(e->key().c_str(),NULL,16);
    _pcontrol.set_transition_env(TransitionId::Configure, _run_key);
    printf("Set run key to %d\n",_run_key);
  }
}

void ConfigSelect::update()
{
  read_db();
  _pcontrol.reconfigure();
}

void ConfigSelect::read_db()
{
  QString type(_runType->currentText());

  //  _expt.read();
  bool ok = 
    disconnect(_runType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(set_run_type(const QString&)));
  _runType->clear();
  const list<TableEntry>& l = _expt.table().entries();
  for(list<TableEntry>::const_iterator iter = l.begin(); iter != l.end(); ++iter) {
    QString str(iter->name().c_str());
    _runType->addItem(str);
  }
  if (ok) 
    connect(_runType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(set_run_type(const QString&)));

  int index=0;
  for(list<TableEntry>::const_iterator iter = l.begin(); iter != l.end(); ++iter, ++index) {
    QString str(iter->name().c_str());
    if (str == type) {
      _runType->setCurrentIndex(index);
      return;
    }
  }
  _runType->setCurrentIndex(0);
}

void ConfigSelect::configured(bool v)
{
  _runType->setEnabled(!v);
}

void ConfigSelect::run_scan(int key)
{
  //  Change key
  _run_key = key;
  _pcontrol.set_transition_env(TransitionId::Configure, _run_key);
  
  //  Run
  _pcontrol.reconfigure();
}

void ConfigSelect::enable_scan(bool l)
{
  _scan->setVisible(l);
  if (!l) update();  // refresh the run key from "Type"
}
