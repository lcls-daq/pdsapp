#include "PartitionSelect.hh"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <set>
#include "pdsapp/control/AliasPoll.hh"
#include "pdsapp/control/SelectDialog.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/ioc/IocControl.hh"
#include "pds/collection/Node.hh"
#include "pds/config/XtcClient.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/BldBitMask.hh"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>

#include <new>
#include <list>
#include <vector>
#include <map>
using std::list;
using std::vector;

//#define DBUG

using namespace Pds;

static const BldBitMask ONE_BIT = BldBitMask(BldBitMask::ONE);

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

  _selectb->setEnabled(false);
}

PartitionSelect::~PartitionSelect()
{
  if (_display)
    _display->close();
  if (_alias_poll)
    delete _alias_poll;
}

void PartitionSelect::attached()
{
  _alias_poll = new AliasPoll(_pcontrol);
  _selectb->setEnabled(true);
}

void PartitionSelect::select_dialog()
{
  if (_alias_poll) {
    delete _alias_poll;
    _alias_poll = 0;
  }

  if (_pcontrol.current_state()!=PartitionControl::Unmapped) {
    if (QMessageBox::question(this,
            "Select Partition",
            "Partition currently allocated. Deallocate partition?\n [This will stop the current run]",
            QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
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

    _detectors   = dialog->detectors();
    _iocs        = dialog->iocs();
    _deviceNames = dialog->deviceNames();
    _segments    = dialog->segments ();
    _reporters   = dialog->reporters();
    _aliases     = dialog->aliases();

    unsigned options(_options);
    float    l3_unbias(0.);
    if (dialog->l3_tag ()) options |= Allocation::L3Tag;
    if (dialog->l3_veto()) { 
      options |= Allocation::L3Veto;
      l3_unbias = dialog->l3_unbias();
    }
    std::string l3_path(dialog->l3_path());

    std::list<DetInfo> inodes;
    for(int i=0; i<_iocs.size(); i++)
      inodes.push_back(_iocs[i]);

    BldBitMask bld_mask = BldBitMask();
    foreach(BldInfo n, _reporters) {
      bld_mask |= ONE_BIT<<n.type();
    }

    BldBitMask bld_mask_mon = BldBitMask();
    QList<BldInfo> transients = dialog->transients();
    foreach(BldInfo n, transients) {
      bld_mask_mon |= ONE_BIT<<n.type();
    }

    if (_validate(bld_mask)) {
      unsigned nsrc=0;
      const std::list<NodeMap>& map = dialog->segment_map();
      for(std::list<NodeMap>::const_iterator it=map.begin();
          it!=map.end(); it++)
	if (!it->node.transient())
	  nsrc += it->sources.size();

      //  Find the master
      unsigned masterid=0;
      foreach(Node node, nodes) {
	if (node.level()==Level::Segment) {
	  for(std::list<NodeMap>::const_iterator it=map.begin();
	      it!=map.end(); it++)
	    if (it->node == node) {
	      const DetInfo info = static_cast<const DetInfo&>(it->sources[0]);
	      masterid = info.devId();
	      break;
	    }
	  break;
	}
      }

      Partition::Source* sources = new Partition::Source[nsrc];
      unsigned isrc=0;

      for(std::list<NodeMap>::const_iterator it=map.begin();
          it!=map.end(); it++) {
#ifdef DBUG
        printf("Segment %08x.%08x contains ",
               it->node.procInfo().log(),
               it->node.procInfo().phy());
#endif
	if (!it->node.transient()) {
	  for(std::vector<Pds::Src>::const_iterator sit=it->sources.begin();
	      sit!=it->sources.end(); sit++) {
#ifdef DBUG
	    printf(" [%08x.%08x]",sit->log(),sit->phy());
#endif
	    const DetInfo info = static_cast<const DetInfo&>(*sit);

	    if (sit->level()==Pds::Level::Source &&
		info.detector()==DetInfo::BldEb)
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
        }
#ifdef DBUG
        printf("\n");
#endif
      }

      unsigned mask_num = PDS_BLD_MASKSIZE;
      unsigned sz = sizeof(PartitionConfigType)+isrc*sizeof(Partition::Source)+mask_num*sizeof(uint32_t);
      char* buff = new char[sz];

      BldBitMask cfg_mask = bld_mask&~bld_mask_mon;
      uint32_t mask_vals[mask_num];
      for (unsigned i=0; i<mask_num; i++) {
        mask_vals[i] = cfg_mask.value(i);
      }

      PartitionConfigType* cfg = new(buff)
        PartitionConfigType(mask_num,isrc,mask_vals,sources);

      {
	char* partn = new char[cfg->_sizeof()];
	new (partn) PartitionConfigType(*cfg);
	Pds_ConfigDb::GlobalCfg::instance().cache(_partitionConfigType,partn,true);
      }

      _icontrol.set_partition(inodes);
      _pcontrol.set_partition(_pt_name, _db_path,
                              l3_path.c_str(),
                              _nodes  , _nnodes,
                              masterid,
                              &bld_mask, &bld_mask_mon,
                              options, 
                              l3_unbias,
                              cfg,
                              dialog->evrio  ().config(dialog->aliases(),cfg),
                              //  Monitoring needs the aliases for Monitor-only data
                              //dialog->aliases().config(cfg));
                              dialog->aliases().config());
      _pcontrol.set_target_state(PartitionControl::Configured);
      
      Pds_ConfigDb::GlobalCfg::instance().cache(_evrIOConfigType, 
						reinterpret_cast<char*>(dialog->evrio  ().config(dialog->aliases())), 
						true);
      Pds_ConfigDb::GlobalCfg::instance().cache(_aliasConfigType, 
						reinterpret_cast<char*>(dialog->aliases().config()),
						true);

      delete[] sources;
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

const AliasFactory&    PartitionSelect::aliases  () const { return _aliases; }

const QList<DetInfo >& PartitionSelect::detectors() const { return _detectors; }

const std::set<std::string>& PartitionSelect::deviceNames() const { return _deviceNames; }

const QList<ProcInfo>& PartitionSelect::segments () const { return _segments ; }

const QList<BldInfo >& PartitionSelect::reporters() const { return _reporters ; }

bool PartitionSelect::_validate(const BldBitMask& bld_mask)
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
  std::set<std::string> unique_names;
  foreach(DetInfo info, _detectors) {
    lEvr     |= (info.device  ()==DetInfo::Evr);
    lBld     |= (info.detector()==DetInfo::BldEb);
    // check for duplicate device name
    const char* p = DetInfo::name(info);
    if (!(unique_names.insert(p).second)) {
      lError = true;
      errorMsg += QString("Duplicate devices selected: %1\n").arg(p);
    }
  }

  bool lXTCav  =false;
  foreach(DetInfo info, _iocs) {
    // check if one of the detectors has an alias "xtcav"
    const char* alias = _aliases.lookup(info);
    if (alias) printf("alias is %s\n", alias);
    else printf("no alias\n");
    lXTCav   |= ((alias != NULL) && (strcmp(alias, "xtcav") == 0));
    // check for duplicate device name
    const char* p = DetInfo::name(info);
    printf("ioc name: %s\n", p);
    if (!(unique_names.insert(p).second)) {
      lError = true;
      errorMsg += QString("Duplicate devices selected: %1 [IOC]\n").arg(p);
    }
  }

  bool lEBeam  =false;
  bool lGasDet =false;
  foreach(BldInfo info, _reporters) {
    // check if EBeam and FEEGasDetEnergy are selected
    lEBeam   |= (info.type()==BldInfo::EBeam);
    lGasDet  |= (info.type()==BldInfo::FEEGasDetEnergy);
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
  if (!lBld && bld_mask.isNotZero()) {
    lWarning = true;
    warnMsg += "Beamline Data selected but not BldEb.\n";
  }

  if (lXTCav && (!lEBeam || !lGasDet)) {
    lWarning = true;
    warnMsg += "XTCav selected but not EBeam and FEEGasDetEnergy.\n";
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
  if (!uRunKey)
    return false;

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
    delete cl;
    return false;
  }

  int iSizeRead = cl->getXTC(uRunKey, det, _evrConfigType, lcConfigBuffer, iMaxEvrDataSize);
  if (iSizeRead <= 0 )
  {
    printf("PartitionSelect::_checkReadGroupEnable():: Read failed. Error: %s\n", strerror(errno));
    free(lcConfigBuffer);
    delete cl;
    return false;
  }

  EvrConfigType& evrConfig  = (EvrConfigType&) *(EvrConfigType*) lcConfigBuffer;

  printf("Evr config event %d pulse %d output %d\n",
    evrConfig.neventcodes(), evrConfig.npulses(), evrConfig.noutputs() );

  bool bEnableReadoutGroup = false;
  for(unsigned i=0; i<evrConfig.neventcodes(); i++)
  {
    const EventCodeType& e = evrConfig.eventcodes()[i];
    if (e.readoutGroup() > 1)
      bEnableReadoutGroup = true;
  }

  free(lcConfigBuffer);
  delete cl;

  return bEnableReadoutGroup;
}

void PartitionSelect::autorun()
{
  _autorun=true;
  select_dialog();
}

void PartitionSelect::latch_aliases()
{
  if (_alias_poll) {
    Pds_ConfigDb::GlobalCfg::instance().cache(_evrIOConfigType, 
					      reinterpret_cast<char*>(_alias_poll->evrio  ().config(_alias_poll->aliases())), 
					      true);
    
    Pds_ConfigDb::GlobalCfg::instance().cache(_aliasConfigType, 
                                              reinterpret_cast<char*>(_alias_poll->aliases().config()),
                                              true);
    delete _alias_poll;
    _alias_poll=0;
  }
}
