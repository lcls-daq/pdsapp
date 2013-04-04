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

#include <errno.h>

using namespace Pds;
using Pds_ConfigDb::Experiment;
using Pds_ConfigDb::Table;
using Pds_ConfigDb::TableEntry;

ConfigSelect::ConfigSelect(QWidget*          parent,
			   PartitionControl& control,
			   const char*       db_path) :
  QGroupBox("Configuration",parent),
  _pcontrol(control),
  _expt    (Pds_ConfigDb::Path(db_path)),
  _control_busy(false)
{
  pthread_mutex_init(&_control_mutex, NULL);
  pthread_cond_init (&_control_cond, NULL);

  _expt.read();

  _reconfig = new Pds_ConfigDb::Reconfig_Ui(this, _expt);
  _scan     = new Pds_ConfigDb::ControlScan(this, _expt);

  _bEdit = new QPushButton("Edit");
  _bScan = new QPushButton("Scan");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(new QLabel("Type"));
    layout1->addWidget(_runType = new QComboBox);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(_bEdit);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(_bScan);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(_bEdit  , SIGNAL(clicked()),                 _reconfig, SLOT(show()));
  connect(_bScan  , SIGNAL(clicked(bool)),	       this, SLOT(enable_scan(bool)));
  connect(this    , SIGNAL(control_enabled(bool)),     this, SLOT(enable_control_(bool)));
  connect(_runType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(set_run_type(const QString&)));
  connect(_reconfig,SIGNAL(changed()),                 this, SLOT(update()));
  connect(_scan    ,SIGNAL(reconfigure()),             this, SLOT(update()));
  connect(_scan    ,SIGNAL(deactivate()),              _bScan, SLOT(click()));

  _read_db();
  set_run_type(_runType->currentText());

  _bScan->setCheckable(true);

  _readSettings();

  _bScan->setChecked  (_scanIsActive);
}

ConfigSelect::~ConfigSelect() 
{
}

void ConfigSelect::enable_control(bool v)
{
  pthread_mutex_lock(&_control_mutex);
  _control_busy = true;

  emit control_enabled(v);

  while(_control_busy)
    pthread_cond_wait(&_control_cond, &_control_mutex);
  pthread_mutex_unlock(&_control_mutex);
}

void ConfigSelect::enable_control_(bool v)
{
  if (!v) {
    _reconfig->enable(false);
    _bScan->setEnabled(false);
    enable_scan(false);
  }
  else {
    _reconfig->enable(true);
    _bScan->setEnabled(true);
    if (_bScan->isChecked())
      enable_scan(true);
    else
      update();
  }
  _control_busy = false;
  pthread_cond_signal(&_control_cond);
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

string ConfigSelect::getType()
{
  return qPrintable(_runType->currentText());
}

void ConfigSelect::update()
{
  _read_db();
  if (_scanIsActive) {
    _run_key = _scan->update_key();
  }
  else {
    const TableEntry* e = _expt.table().get_top_entry(qPrintable(_runType->currentText()));
    _run_key = strtoul(e->key().c_str(),NULL,16);
  }
  _pcontrol.set_transition_env(TransitionId::Configure, _run_key);
  _pcontrol.reconfigure();
}

void ConfigSelect::_read_db()
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
  _writeSettings();
}

void ConfigSelect::enable_scan(bool l)
{
  _scan->setVisible(l);
  _scanIsActive = l;
  if (!l) update();  // refresh the run key from "Type"
}

void ConfigSelect::_writeSettings()
{
  char buff[64];
  snprintf(buff, sizeof(buff)-1, ".%s for platform %u", qPrintable(title()), _pcontrol.header().platform());
  FILE* f = fopen(buff,"w");
  if (f) {
    fprintf(f,"%s\n",qPrintable(_runType->currentText()));
    fprintf(f,"%s\n",_scanIsActive ? "scan":"no_scan");
    fclose(f);
  }
  else {
    printf("Failed to open %s\n", buff);
  }
}

static const unsigned SETTINGS_SIZE = 64;

void ConfigSelect::_readSettings()
{
  _scanIsActive=false;

  char *buff = (char *)malloc(SETTINGS_SIZE);  // use malloc w/ getline
  if (buff == (char *)NULL) {
    printf("%s: malloc(%d) failed, errno=%d\n", __PRETTY_FUNCTION__, SETTINGS_SIZE, errno);
  } else {
    snprintf(buff, SETTINGS_SIZE-1, ".%s for platform %u", qPrintable(title()), _pcontrol.header().platform());
    FILE* f = fopen(buff,"r");
    if (f) {
      printf("Opened %s\n",buff);
      char* lptr=buff;
      size_t linesz = SETTINGS_SIZE;         // initialize for getline
      if (getline(&lptr,&linesz,f)!=-1) {
        QString p(lptr);
        p.chop(1);  // remove new-line
        int index = _runType->findText(p);
        if (index >= 0)
          _runType->setCurrentIndex(index);
      }
      if (getline(&lptr,&linesz,f)!=-1 &&
          strcmp(lptr,"scan")==0)
        _scanIsActive = true;

      fclose(f);
    }
    else {
      printf("Failed to open %s\n", buff);
    }
    free(buff);
  }
}
