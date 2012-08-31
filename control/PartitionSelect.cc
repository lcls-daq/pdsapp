#include "PartitionSelect.hh"

#include <fcntl.h>
#include <unistd.h>
#include "pdsapp/control/SelectDialog.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/collection/Node.hh"
#include "pds/config/CfgPath.hh"
#include "pds/config/EvrConfigType.hh"
#include "pdsapp/config/Path.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Table.hh"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>

using namespace Pds;

PartitionSelect::PartitionSelect(QWidget*          parent,
         PartitionControl& control,
         const char*       pt_name,
         const char*       db_path) :
  QGroupBox ("Partition",parent),
  _pcontrol (control),
  _pt_name  (pt_name),
  _display  (0)
{
  sprintf(_db_path,"%s/keys",db_path);
  strncpy(_db_path_org,db_path, sizeof(_db_path_org));

  QPushButton* display;

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->addWidget(_selectb = new QPushButton("Select",this));
  layout->addWidget( display = new QPushButton("Display",this));
  setLayout(layout);

  connect(_selectb, SIGNAL(clicked()), this, SLOT(select_dialog()));
  connect( display, SIGNAL(clicked()), this, SLOT(display()));
}

PartitionSelect::~PartitionSelect() 
{
}

void PartitionSelect::select_dialog()
{
  if (_pcontrol.current_state()!=PartitionControl::Unmapped) {
    if (QMessageBox::question(this,
            "Select Partition",
            "Partition currently allocated. Deallocate partition?\n [This will stop the current run]",
            QMessageBox::Ok | QMessageBox::Cancel)
  == QMessageBox::Ok) {
      _pcontrol.set_target_state(PartitionControl::Unmapped);
      // wait for completion
      sleep(1);
    }
    else
      return;
  }

  bool show = false;
  if (_display) {
    show = !_display->isHidden();
    _display->close();
    _display = 0;
  }

  bool bReadGroupEnable = _checkReadGroupEnable();
  
  SelectDialog* dialog = new SelectDialog(this, _pcontrol, bReadGroupEnable);
  bool ok = dialog->exec();
  if (ok) {
    QList<Node> nodes = dialog->selected();
    _nnodes = 0;
    foreach(Node node, nodes) {
      _nodes[_nnodes++] = node;
    }

    _detectors = dialog->detectors();
    _segments  = dialog->segments ();
    _reporters = dialog->reporters();

    unsigned bld_mask = 0 ;
    foreach(BldInfo n, _reporters) {
      bld_mask |= 1<<n.type();
    }

    if (_validate(bld_mask)) {
      _pcontrol.set_partition(_pt_name, _db_path, _nodes, _nnodes, bld_mask);
      _pcontrol.set_target_state(PartitionControl::Configured);
    }

    _display = dialog->display();
    if (show)
      _display->show();
  }
  delete dialog;
}

void PartitionSelect::display()
{
  if (_display)
    _display->show();
}

void PartitionSelect::change_state(QString s)
{
  if (s == QString(TransitionId::name(TransitionId::Unmap))) _selectb->setEnabled(true);
  if (s == QString(TransitionId::name(TransitionId::Map  ))) _selectb->setEnabled(false);
}

const QList<DetInfo >& PartitionSelect::detectors() const { return _detectors; }

const QList<ProcInfo>& PartitionSelect::segments () const { return _segments ; }

const QList<BldInfo >& PartitionSelect::reporters() const { return _reporters ; }

bool PartitionSelect::_validate(unsigned bld_mask) 
{
  bool lEvent  =false;
  for(unsigned i=0; i<_nnodes; i++) {
    lEvent   |= (_nodes[i].level()==Level::Event);
  }

  bool lEvr    =false;
  bool lBld    =false;
  foreach(DetInfo info, _detectors) {
    lEvr     |= (info.device  ()==DetInfo::Evr);
    lBld     |= (info.detector()==DetInfo::BldEb);
  }

  bool lError  =false;
  bool lWarning=false;

  QString errorMsg;
  if (!lEvent) {
    lError = true;
    errorMsg += "No Processing Node selected.\n";
  }

  if (!lEvr) {
    lError = true;
    errorMsg += "No EVR selected.\n";
  }

  QString warnMsg;
  if (!lBld && bld_mask) {
    lWarning = true;
    warnMsg += "Beamline Data selected but not BldEb.\n";
  }

  if (lError) {
    QMessageBox::critical(this, "Partition Select Error", errorMsg);
  }
  else if (lWarning) {
    if (QMessageBox::warning(this, "Partition Select Warning", warnMsg,
                             QMessageBox::Ok | QMessageBox::Abort) ==
        QMessageBox::Abort) {
      lError = true;
    }
  }

  return !lError;
}

bool PartitionSelect::_checkReadGroupEnable()
{
  Pds_ConfigDb::Experiment expt = Pds_ConfigDb::Experiment(Pds_ConfigDb::Path(_db_path_org));
  expt.read();
  
  unsigned int uRunKey = _pcontrol.get_transition_env(TransitionId::Configure);
  
  const DetInfo det(0,DetInfo::NoDetector,0,DetInfo::Evr,0);
  char strConfigPath[128];
  sprintf(strConfigPath,"%s/keys/%s",_db_path_org,CfgPath::path(uRunKey,det,_evrConfigType).c_str());
  
  int fdConfig = ::open(strConfigPath, O_RDONLY);
  if ( fdConfig == -1 )
  {
    printf("PartitionSelect::_checkReadGroupEnable(): Read Evr config file (%s) failed\n", strConfigPath);      
    return false;
  }
  
  char config[sizeof(EvrConfigType)];
  int iSizeRead = ::read(fdConfig, &config, sizeof(EvrConfigType));
  if (iSizeRead != sizeof(EvrConfigType))
  {
    printf("PartitionSelect::_checkReadGroupEnable():: Read config data of smaller size. Read size = %d (should be %d) bytes\n",
      iSizeRead, sizeof(EvrConfigType));
    return false;
  }

  int iCloseFail = ::close(fdConfig);
  if ( iCloseFail == -1 )
  {
    printf("PartitionSelect::_checkReadGroupEnable(): Close Evr config file (%s) failed\n", strConfigPath);      
    return false;
  }
  
  EvrConfigType& evrConfig = (EvrConfigType&) config;
  printf("Evr config event %d pulse %d output %d readGroup %s\n", 
    evrConfig.neventcodes(), evrConfig.npulses(), evrConfig.noutputs(), 
    (evrConfig.enableReadGroup() != 0 ? "On" : "Off"));

  return (evrConfig.enableReadGroup() != 0);
}
