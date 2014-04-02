#include "PartitionSelect.hh"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <set>
#include "pdsapp/control/SelectDialog.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/ioc/IocControl.hh"
#include "pds/collection/Node.hh"
#include "pds/config/XtcClient.hh"
#include "pds/config/EvrConfigType.hh"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>

#define DBUG

using namespace Pds;

PartitionSelect::PartitionSelect(QWidget*          parent,
                                 PartitionControl& control,
				 IocControl&       icontrol,
                                 const char*       pt_name,
                                 const char*       db_path,
                                 unsigned          options) :
  QGroupBox ("Partition",parent),
  _pcontrol (control),
  _icontrol (icontrol),
  _pt_name  (pt_name),
  _display  (0),
  _options  (options),
  _autorun  (false)
{
  strncpy(_db_path,db_path, sizeof(_db_path));

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
  
  SelectDialog* dialog = new SelectDialog(this, _pcontrol, _icontrol, bReadGroupEnable, _autorun);
  bool ok = dialog->exec();
  if (ok) {
    QList<Node> nodes = dialog->selected();
    _nnodes = 0;
    foreach(Node node, nodes) {
      _nodes[_nnodes++] = node;
    }

    _detectors = dialog->detectors();
    _deviceNames = dialog->deviceNames();
    _segments  = dialog->segments ();
    _reporters = dialog->reporters();
    
    QList<DetInfo> iocs = dialog->iocs();
    std::list<DetInfo> inodes;
    for(int i=0; i<iocs.size(); i++)
      inodes.push_back(iocs[i]);

    uint64_t bld_mask = 0 ;
    foreach(BldInfo n, _reporters) {
      bld_mask |= 1ULL<<n.type();
    }

    uint64_t bld_mask_mon = 0 ;
    QList<BldInfo> transients = dialog->transients();
    foreach(BldInfo n, transients) {
      bld_mask_mon |= 1ULL<<n.type();
    }

    if (_validate(bld_mask)) {
#if 1
      unsigned nsrc=0;
      const std::list<NodeMap>& map = dialog->segment_map();
      for(std::list<NodeMap>::const_iterator it=map.begin();
          it!=map.end(); it++)
        nsrc += it->sources.size();

      Partition::Source* sources = new Partition::Source[nsrc];
      unsigned isrc=0;
      for(std::list<NodeMap>::const_iterator it=map.begin();
          it!=map.end(); it++) {
#ifdef DBUG
        printf("Segment %08x.%08x contains ",
               it->node.procInfo().log(),
               it->node.procInfo().phy());
#endif
        for(std::vector<Pds::Src>::const_iterator sit=it->sources.begin();
            sit!=it->sources.end(); sit++) {
#ifdef DBUG
          printf(" [%08x.%08x]",sit->log(),sit->phy());
#endif
          if (sit->level()==Pds::Level::Source &&
              static_cast<const Pds::DetInfo&>(*sit).detector()==Pds::DetInfo::BldEb)
            continue;

          foreach(Node node, nodes) {
            if (node == it->node) {
              sources[isrc++] = Partition::Source(*sit, node.group());
#ifdef DBUG
              printf("*");
#endif
              break;
            }
          }
        }
#ifdef DBUG
        printf("\n");
#endif
      }

      unsigned sz = sizeof(Partition::ConfigV1)+isrc*sizeof(Partition::Source);
      char* buff = new char[sz];
      
      PartitionConfigType* cfg = new(buff) 
        PartitionConfigType(bld_mask&~bld_mask_mon,isrc,sources);

      _icontrol.set_partition(inodes);
      _pcontrol.set_partition(_pt_name, _db_path, 
                              _nodes  , _nnodes, 
                              bld_mask, bld_mask_mon,
                              _options, cfg);
      _pcontrol.set_target_state(PartitionControl::Configured);

      delete[] buff;
      delete[] sources;
#else
      _icontrol.set_partition(inodes);
      _pcontrol.set_partition(_pt_name, _db_path, 
                              _nodes  , _nnodes, 
                              bld_mask, bld_mask_mon,
                              _options);
      _pcontrol.set_target_state(PartitionControl::Configured);
#endif
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

const std::set<std::string>& PartitionSelect::deviceNames() const { return _deviceNames; }

const QList<ProcInfo>& PartitionSelect::segments () const { return _segments ; }

const QList<BldInfo >& PartitionSelect::reporters() const { return _reporters ; }

bool PartitionSelect::_validate(uint64_t bld_mask) 
{
  bool lEvent  =false;
  bool lError  =false;
  bool lWarning=false;
  QString errorMsg;

  for(unsigned i=0; i<_nnodes; i++) {
    lEvent   |= (_nodes[i].level()==Level::Event);
  }

  bool lEvr    =false;
  bool lBld    =false;
  std::pair<std::set<std::string>::iterator,bool> insertResult;
  std::set<std::string> unique_names;
  foreach(DetInfo info, _detectors) {
    lEvr     |= (info.device  ()==DetInfo::Evr);
    lBld     |= (info.detector()==DetInfo::BldEb);
    // check for duplicate device name
    insertResult = unique_names.insert(DetInfo::name(info));
    if (!(insertResult.second)) {
      lError = true;
      errorMsg += "Duplicate devices selected: ";
      errorMsg += DetInfo::name(info);
      errorMsg += "\n";
    }
  }

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
  unsigned int uRunKey = _pcontrol.get_transition_env(TransitionId::Configure);
  
  const DetInfo det(0,DetInfo::NoDetector,0,DetInfo::Evr,0);

  Pds_ConfigDb::XtcClient* cl = Pds_ConfigDb::XtcClient::open(_db_path);
  if (!cl) {
    printf("PartitionSelect::_checkReadGroupEnable(): failed to get configuration data\n");
    return false;
  }
  
  static const unsigned MaxUserCodes      = 16;
  static const unsigned MaxGlobalCodes    = 4;  
  int iMaxEvrDataSize = sizeof(EvrConfigType) + (MaxUserCodes+MaxGlobalCodes) * sizeof(EventCodeType);
  char* lcConfigBuffer = (char*) malloc( iMaxEvrDataSize );
  if ( lcConfigBuffer == NULL )
  {
    printf("PartitionSelect::_checkReadGroupEnable(): malloc(%d) failed. Error: %s\n", iMaxEvrDataSize, strerror(errno));      
    return false;
  }

  int iSizeRead = cl->getXTC(uRunKey, det, _evrConfigType, lcConfigBuffer, iMaxEvrDataSize);
  if (iSizeRead == -1 )
  {
    printf("PartitionSelect::_checkReadGroupEnable():: Read failed. Error: %s\n", strerror(errno));      
    free(lcConfigBuffer);
    return false;
  }
    
  EvrConfigType& evrConfig  = (EvrConfigType&) *(EvrConfigType*) lcConfigBuffer;    
    
  printf("Evr config event %d pulse %d output %d\n", 
    evrConfig.neventcodes(), evrConfig.npulses(), evrConfig.noutputs() );

  bool bEneableReadoutGroup = false;
  for(unsigned i=0; i<evrConfig.neventcodes(); i++) 
  {
    const EventCodeType& e = evrConfig.eventcodes()[i];
    if (e.readoutGroup() > 1)
      bEneableReadoutGroup = true;
  }
  
  free(lcConfigBuffer);
    
  return bEneableReadoutGroup;
}

void PartitionSelect::autorun()
{
  _autorun=true;
  select_dialog();
}
