#include "pdsapp/control/ConfigSelect.hh"
#include "pdsapp/control/Preferences.hh"
#include "pdsapp/control/SequencerSync.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Reconfig_Ui.hh"
#include "pdsapp/config/ControlScan.hh"
#include "pds/management/PartitionControl.hh"

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <errno.h>

using namespace Pds;
using Pds_ConfigDb::Experiment;
using Pds_ConfigDb::Table;
using Pds_ConfigDb::TableEntry;

static void* _attach(void* arg)
{
  ConfigSelect* c = (ConfigSelect*)arg;
  c->attach();
  return arg;
}

static const char* pref_name = "Configuration";
static const char* seq_name  = "sequencer";

ConfigSelect::ConfigSelect(QWidget*          parent,
			   PartitionControl& control,
			   const char*       db_path) :
  QGroupBox("Configuration (LOCKED)",parent),
  _pcontrol(control),
  _db_path (db_path),
  _expt    (0),
  _reconfig(0),
  _scan    (0),
  _seq     (0),
  _control_busy(false)
{
  setPalette(QPalette(::Qt::yellow));

  pthread_mutex_init(&_control_mutex, NULL);
  pthread_cond_init (&_control_cond, NULL);

  _bEdit = new QPushButton("Edit");
  _bScan = new QPushButton("Scan");

  _cSeq  = new QCheckBox;
  _bSeq  = new QComboBox;
  for(unsigned i=1; i<9; i++)
    _bSeq->addItem(QString("%1").arg(i));

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
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(_cSeq);
    layout1->addWidget(new QLabel("Sync Sequence"));
    layout1->addWidget(_bSeq);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(_bEdit  , SIGNAL(clicked()),                 this, SLOT(edit_config()));
  connect(_bScan  , SIGNAL(clicked(bool)),	       this, SLOT(enable_scan(bool)));
  connect(this    , SIGNAL(control_enabled(bool)),     this, SLOT(enable_control_(bool)));
  connect(_runType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(set_run_type(const QString&)));
  connect(this    , SIGNAL(_attached()),               this, SLOT(attached()));

  setEnabled  (false);

  pthread_t threadId;
  pthread_create(&threadId,NULL,_attach,this);
}

ConfigSelect::~ConfigSelect() 
{
  if (_reconfig)
    delete _reconfig;
  if (_scan)
    delete _scan;
  if (_expt)
    delete _expt;
}

void ConfigSelect::edit_config()
{
  if (_expt) {
    emit aliases_required();
    _reconfig->show();
  }
}

void ConfigSelect::enable_control(bool v)
{
  if (v)
    _open_db();
    
  pthread_mutex_lock(&_control_mutex);
  _control_busy = true;

  emit control_enabled(v);

  while(_control_busy)
    pthread_cond_wait(&_control_cond, &_control_mutex);
  pthread_mutex_unlock(&_control_mutex);


  if (!v) {
    delete _expt;
    _expt = 0;
  }
}

void ConfigSelect::enable_control_(bool v)
{
  if (!v) {
    setEnabled(false);

    _bScan->setEnabled(false);

    if (_reconfig) {
      delete _reconfig;
      _reconfig = 0;
    }

    if (_scan) {
      delete _scan;
      _scan = 0;
    }
  }
  else {
    _reconfig = new Pds_ConfigDb::Reconfig_Ui(this, *_expt);
    connect(_reconfig,SIGNAL(changed()), this, SLOT(update()));
  
    _scan = new Pds_ConfigDb::ControlScan(this, *_expt);
    connect(_scan    ,SIGNAL(reconfigure()),             this, SLOT(update()));
    connect(_scan    ,SIGNAL(deactivate()),              _bScan, SLOT(click()));

    setEnabled(true);

    _bScan->setEnabled(true);
    _scan->setVisible(_bScan->isChecked());

    update();
  }
  _control_busy = false;
  pthread_cond_signal(&_control_cond);
}

void ConfigSelect::set_run_type(const QString& run_type)
{
  if (_reconfig)
    _reconfig->set_run_type(run_type);
  else
    printf("ConfigSelect _reconfig null\n");

  if (_scan)
    _scan    ->set_run_type(run_type);
  else
    printf("ConfigSelect _scan null\n");

  const TableEntry* e = _expt->table().get_top_entry(qPrintable(run_type));
  if (e) {
    _run_key = strtoul(e->key().c_str(),NULL,16);
    _pcontrol.set_transition_env(TransitionId::Configure, _run_key);
    printf("Set run key to %d\n",_run_key);
  }
}

bool ConfigSelect::controlpvs() const 
{
  return (_bScan->isEnabled() && _bScan->isChecked() && _scan->pvscan());
}

string ConfigSelect::getType()
{
  return qPrintable(_runType->currentText());
}

void ConfigSelect::update()
{
  if (_bScan->isEnabled() && _bScan->isChecked()) {
    _reconfig->setEnabled(false);
    _run_key = _scan->update_key();
  }
  else {
    _reconfig->setEnabled(true);
    _read_db();
    set_run_type(_runType->currentText());
  }
  printf("Reconfigure with run key 0x%x\n",_run_key);
  _pcontrol.set_transition_env(TransitionId::Configure, _run_key);
  _pcontrol.reconfigure();
}

void ConfigSelect::_open_db()
{
  bool asserted=false;
  while(1) {
    try {
      _expt = new Pds_ConfigDb::Experiment(_db_path);
    }
    catch(std::string& serr) {
      if (!asserted) {
        asserted = true;
        setTitle(QString("Configuration (%1)").arg(serr.c_str()));
        setPalette(QPalette(::Qt::yellow));
      }
      delete _expt;
      sleep(1);
      continue;
    }
    
    if (asserted) {
      setTitle("Configuration");
      setPalette(QPalette());
    }
    break;
  }  

  _expt->read();
}

void ConfigSelect::_read_db()
{
  QString type(_runType->currentText());

  //  _expt.read();
  bool ok = 
    disconnect(_runType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(set_run_type(const QString&)));
  _runType->clear();
  const list<TableEntry>& l = _expt->table().entries();
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
  _cSeq   ->setEnabled(!v);
  _bSeq   ->setEnabled(!v);
  _writeSettings();
}

void ConfigSelect::configured_(bool v)
{
  if (_seq) { delete _seq; _seq=0; }
  if (_cSeq->checkState()==::Qt::Checked)
    _seq = new SequencerSync(_bSeq->currentIndex()+1);
  _pcontrol.set_sequencer(_seq);
}

void ConfigSelect::enable_scan(bool l)
{
  if (_scan)
    _scan->setVisible(l);

  update();
}

void ConfigSelect::_writeSettings()
{
  Preferences pref(pref_name,
                   _pcontrol.header().platform(),
                   "w");
  pref.write(_runType->currentText());
  pref.write(_bScan->isEnabled() && _bScan->isChecked() ? "scan":"no_scan");
  pref.write(QString("%1%2")
	     .arg(_cSeq->checkState()==::Qt::Checked ? "":"no ")
	     .arg(seq_name),
	     qPrintable(_bSeq->currentText()));
}

static const unsigned SETTINGS_SIZE = 64;

void ConfigSelect::_readSettings()
{
  _bScan->setChecked(false);
  
  Preferences pref(pref_name,
		   _pcontrol.header().platform(),
		   "r");
  QList<QString> l;
  pref.read(l);

  if (l.size()>0) {
    int index = _runType->findText(l[0]);
    if (index >= 0)
      _runType->setCurrentIndex(index);
    
    if (l.size()>1 && l[1]=="scan")
      _bScan->setChecked(true);
  }
}

void ConfigSelect::attach()
{
  _open_db();

  emit _attached();
}

void ConfigSelect::attached()
{
  _reconfig = new Pds_ConfigDb::Reconfig_Ui(this, *_expt);
  connect(_reconfig,SIGNAL(changed()), this, SLOT(update()));
    
  _scan = new Pds_ConfigDb::ControlScan(this, *_expt);
  connect(_scan    ,SIGNAL(reconfigure()),             this, SLOT(update()));
  connect(_scan    ,SIGNAL(deactivate()),              _bScan, SLOT(click()));
    
  _read_db();
  set_run_type(_runType->currentText());
    
  _bScan->setCheckable(true);
    
  _readSettings();
    
  setTitle("Configuration");
  setPalette(QPalette());
  setEnabled(true);
}
