#include "SelectDialog.hh"

#include "pdsdata/xtc/SegmentInfo.hh"
#include "pdsapp/control/DetNodeGroup.hh"
#include "pdsapp/control/BldNodeGroup.hh"
#include "pdsapp/control/ProcNodeGroup.hh"
#include "pdsapp/control/NodeSelect.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/ioc/IocControl.hh"
#include "pds/collection/PingReply.hh"
#include "pds/collection/AliasReply.hh"
#include "pds/utility/StreamPorts.hh"

using namespace Pds;

static bool _useTransient = false;

SelectDialog::SelectDialog(QWidget* parent,
			   PartitionControl& control, 
			   IocControl& icontrol, 
			   bool bReadGroupEnable,
			   bool autorun) :
  QDialog  (parent),
  _pcontrol(control),
  _icontrol(icontrol),
  _bReadGroupEnable(bReadGroupEnable),
  _autorun (autorun)
{
  setWindowTitle("Partition Selection");

  unsigned platform = _pcontrol.header().platform();
  QGridLayout* layout = new QGridLayout(this);
  layout->addWidget(_segbox = new DetNodeGroup("Readout Nodes",this, platform, 
					       _bReadGroupEnable, _useTransient ), 
                    0, 0);
  layout->addWidget(_evtbox = new ProcNodeGroup("Processing Nodes",this, platform), 
                    1, 0);
  layout->addWidget(_rptbox = new BldNodeGroup("Beamline Data",this, platform,
					       0, _useTransient ),
                    2, 0);
  layout->addWidget(_iocbox = new DetNodeGroup("Camera IOCs",this, platform),
		    3, 0);

  _acceptb = new QPushButton("Ok",this);
  QPushButton* rejectb = new QPushButton("Cancel",this);
  QHBoxLayout* layoutb = new QHBoxLayout;
  layoutb->addWidget(_acceptb);
  layoutb->addWidget(rejectb);
  layout->addLayout(layoutb, 3, 0, 1, 3);
  setLayout(layout);

  connect(this    , SIGNAL(changed()), this, SLOT(update_layout()));
  connect(_acceptb, SIGNAL(clicked()), this, SLOT(select()));
  connect( rejectb, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_segbox , SIGNAL(list_changed()), this, SLOT(check_ready()));
  connect(_evtbox , SIGNAL(list_changed()), this, SLOT(check_ready()));

  _pcontrol.platform_rollcall(this);
  _icontrol.host_rollcall(this);
}

SelectDialog::~SelectDialog() 
{
  _pcontrol.platform_rollcall(0);
  _icontrol.host_rollcall(0);
}

void        SelectDialog::available(const Node& hdr, const PingReply& msg) 
{
  std::vector<Src> sources(msg.nsources());
  for(unsigned i=0; i<msg.nsources(); i++)
    sources[i] = msg.source(i);
  switch(hdr.level()) {
  case Level::Control : _control = hdr; break;
  case Level::Segment : 
    {
      std::vector<Src> parents;
      std::vector<Src> children;
      _segment_map.push_back(NodeMap(hdr,sources));

      for(unsigned i=0; i<msg.nsources(); i++) 
      {
        if (msg.source(i).level()==Level::Reporter) 
        {
          const BldInfo& bld = static_cast<const BldInfo&>(msg.source(i));
          Node h(hdr);
          h.fixup(StreamPorts::bld(bld.type()).address(),h.ether());
          _rptbox->addNode(NodeSelect(h,bld));
        }
        else {
          if (hdr.triggered())
            _evrio.insert(hdr.evr_module(),
                          hdr.evr_channel(),
                          static_cast<const DetInfo&>(msg.source(i)));

          // Check if the Node is part of a mutli-node detector
          const SegmentInfo& info = reinterpret_cast<const SegmentInfo&>(msg.source(i));
          if (info.isChild()) {
            parents.push_back(SegmentInfo::parent(info, false));
            children.push_back(msg.source(i));
          }
        }
      }
      const char *aliasName;
      std::string strAliasName;

      if ((msg.nsources() > 0) && (aliasName = _aliases.lookup(msg.source(0)))) {
        char namebuf[msg.nsources() * (SrcAlias::AliasNameMax + 5)];
        char *pName = namebuf;

        // if multiple aliases are present, display them in a vertical list
        for (int ii=0; ii<(int)msg.nsources(); ii++) {
          aliasName = _aliases.lookup(msg.source(ii));
          if (aliasName && (strlen(aliasName) <= SrcAlias::AliasNameMax)) {
            if (ii > 0) {
              strcpy(pName, "\r\n");
              pName += 2;
            }
            strcpy(pName, aliasName);
            pName += strlen(aliasName);
          } else {
            printf("%s:%d skipped alias #%d (len=%d) ...\n", __FILE__, __LINE__, ii,
                   (int) (aliasName ? strlen(aliasName) : 0));
          }
        }

        // if multiple aliases are present, display them in a vertical list
        char parentbuf[msg.nsources() * (SrcAlias::AliasNameMax + 5)];
        pName = parentbuf;
        for (int ii=0; ii<(int)parents.size(); ii++) {
          strAliasName.assign(_aliases.lookup(children[ii]));
          strAliasName.assign(strAliasName, 0, strAliasName.rfind('_'));
          if (!strAliasName.empty() && (strAliasName.length() <= SrcAlias::AliasNameMax)) {
            if (ii > 0) {
              strcpy(pName, "\r\n");
              pName += 2;
            }
            strcpy(pName, strAliasName.c_str());
            pName += strAliasName.length();
          } else {
            printf("%s:%d skipped alias #%d (len=%d) ...\n", __FILE__, __LINE__, ii,
                   (int)  strAliasName.length());
          }
        }

        if (!parents.empty()) {
          NodeSelect parent(Node(hdr.level(), hdr.platform()), msg.ready(), parents, QString(parentbuf));
          NodeSelect child(hdr, msg.ready(), sources, QString(namebuf));
          _segbox->addChildNode(parent, child);
        } else {
          _segbox->addNode(NodeSelect(hdr, msg.ready(), sources, QString(namebuf)));
        }
      } else {
        if (!parents.empty()) {
          NodeSelect parent(Node(hdr.level(), hdr.platform()), msg.ready(), parents);
          NodeSelect child(hdr, msg.ready(), sources);
          _segbox->addChildNode(parent, child);
        } else {
          _segbox->addNode(NodeSelect(hdr, msg.ready(), sources));
        }
      }
      break; 
    }
  case Level::Event   : _evtbox->addNode(NodeSelect(hdr, msg.ready(), sources)); break;
  default: break;
  }

  emit changed();
}

void        SelectDialog::connected(const IocNode& ioc) {
  _iocbox->addNode(NodeSelect(ioc));
  emit changed();
}

void        SelectDialog::aliasCollect(const Node& hdr, const AliasReply& msg)
{
  if (hdr.level() == Level::Segment) {
    int count = (int)msg.naliases();
    for (int ix=0; ix < count; ix++) {
      _aliases.insert(msg.alias(ix));
    }
  }
}

void SelectDialog::update_layout()
{
  //
  //  Balance the layout
  //
  const unsigned MaxRows = 15;
  unsigned ns = _segbox->nodes();
  unsigned ne = _evtbox->nodes();
  unsigned nr = _rptbox->nodes();
  unsigned ni = _iocbox->nodes();
  QGridLayout* layout = static_cast<QGridLayout*>(this->layout());
  if (ns+ne>MaxRows) {
    _clearLayout();
    if (ne+nr > MaxRows) {
      if (nr+ni > MaxRows) {
	layout->addWidget(_segbox,0,0);
	layout->addWidget(_evtbox,0,1);
	layout->addWidget(_rptbox,0,2);
	layout->addWidget(_iocbox,0,3);
      }
      else {
	layout->addWidget(_segbox,0,0,2,1);
	layout->addWidget(_evtbox,0,1,2,1);
	layout->addWidget(_rptbox,0,2);
	layout->addWidget(_iocbox,1,2);
      }
    }
    else {
      if (ne+nr+ni > MaxRows) {
	if (ne > ni) {
	  layout->addWidget(_segbox,0,0,2,1);
	  layout->addWidget(_evtbox,0,1,2,1);
	  layout->addWidget(_rptbox,0,2);
	  layout->addWidget(_iocbox,1,2);
	}
	else {
	  layout->addWidget(_segbox,0,0,2,1);
	  layout->addWidget(_evtbox,0,1);
	  layout->addWidget(_rptbox,1,1);
	  layout->addWidget(_iocbox,0,2,2,1);
	}
      }
      else {
	layout->addWidget(_segbox,0,0,3,1);
	layout->addWidget(_evtbox,0,1);
	layout->addWidget(_rptbox,1,1);
	layout->addWidget(_iocbox,2,1);
      }
    }
  }
  else if (ns+ne+nr > MaxRows) {
    _clearLayout();
    if (nr+ni > MaxRows) {
      layout->addWidget(_segbox,0,0);
      layout->addWidget(_evtbox,1,0);
      layout->addWidget(_rptbox,0,1,2,1);
      layout->addWidget(_iocbox,0,2,2,1);
    }
    else {
      layout->addWidget(_segbox,0,0);
      layout->addWidget(_evtbox,1,0);
      layout->addWidget(_rptbox,0,1);
      layout->addWidget(_iocbox,1,1);
    }
  }
  else if (ns+ne+nr+ni > MaxRows) {
    if (ns > ni) {
      layout->addWidget(_segbox,0,0,3,1);
      layout->addWidget(_evtbox,0,1);
      layout->addWidget(_rptbox,1,1);
      layout->addWidget(_iocbox,2,1);
    }
    else {
      layout->addWidget(_segbox,0,0);
      layout->addWidget(_evtbox,1,0);
      layout->addWidget(_rptbox,2,0);
      layout->addWidget(_iocbox,0,1,3,1);
    }
  }
  else {
      layout->addWidget(_segbox,0,0);
      layout->addWidget(_evtbox,1,0);
      layout->addWidget(_rptbox,2,0);
      layout->addWidget(_iocbox,3,0);
  }
  updateGeometry();
}

const AliasFactory&    SelectDialog::aliases  () const { return _aliases; }

const EvrIOFactory&    SelectDialog::evrio    () const { return _evrio; }

const QList<Node    >& SelectDialog::selected () const { return _selected; }

const QList<DetInfo >& SelectDialog::detectors() const { return _detinfo; }

const std::set<std::string>& SelectDialog::deviceNames() const { return _deviceNames; }

const QList<ProcInfo>& SelectDialog::segments () const { return _seginfo; }

const QList<BldInfo >& SelectDialog::reporters() const { return _rptinfo; }

const QList<BldInfo >& SelectDialog::transients() const { return _trninfo; }

const QList<DetInfo >& SelectDialog::iocs      () const { return _iocinfo; }

const std::list<NodeMap>& SelectDialog::segment_map() const { return _segment_map; }

bool        SelectDialog::l3_tag () const 
{
  return _evtbox->useL3F ()&&~_evtbox->useVeto(); 
}

bool        SelectDialog::l3_veto() const 
{
  return _evtbox->useL3F ()&& _evtbox->useVeto(); 
}

std::string SelectDialog::l3_path() const 
{
  return std::string(qPrintable(_evtbox->inputData()));
}

float SelectDialog::l3_unbias() const
{
  return _evtbox->unbiased_fraction();
}

QWidget* SelectDialog::display() {
  QWidget* d = new QWidget((QWidget*)0);
  d->setAttribute(Qt::WA_DeleteOnClose,false);
  d->setWindowTitle("Partition Display");
  QVBoxLayout* layout = new QVBoxLayout(d);
  layout->addWidget(_segbox->freeze()); 
  layout->addWidget(_evtbox->freeze()); 
  layout->addWidget(_rptbox->freeze()); 
  layout->addWidget(_iocbox->freeze()); 
  d->setLayout(layout);
  return d;
}

void SelectDialog::select() {
  _selected.clear();
  _selected << _control;
  _selected << _segbox->selected();
  _selected << _evtbox->selected();
  _rptbox->selected();
  _iocbox->selected();

  _detinfo << _segbox->detectors();
  _deviceNames = _segbox->deviceNames();

  _rptinfo = _rptbox->reporters();
  _trninfo = _rptbox->transients();

  std::list<NodeMap> new_segment_map;
  foreach(Node n, _segbox->selected()) {
    _seginfo  << n.procInfo();
    for(std::list<NodeMap>::const_iterator it=_segment_map.begin();
	it!=_segment_map.end(); it++) {
      if (it->node==n) {
	std::vector<Src> sources;
	for(std::vector<Src>::const_iterator sit=it->sources.begin();
	    sit!=it->sources.end(); sit++) {
	  if (sit->level()==Level::Reporter) {
	    const BldInfo& bit = static_cast<const BldInfo&>(*sit);
	    if (!_rptinfo.contains(bit) ||
		_trninfo.contains(bit))
	      continue;
	  }
	  sources.push_back(*sit);
	}
	new_segment_map.push_back(NodeMap(n,sources));
      }
    }
  }
  _segment_map = new_segment_map;

  _iocinfo = _iocbox->detectors();

  accept();
}

void SelectDialog::check_ready()
{
  bool enable = _segbox->ready() && _evtbox->ready();
  _acceptb->setEnabled(enable);
  if (_autorun && enable) {
    _autorun = false;
    _acceptb->click();
  }
}

void SelectDialog::_clearLayout()
{
  layout()->removeWidget(_segbox);
  layout()->removeWidget(_evtbox);
  layout()->removeWidget(_rptbox);
  layout()->removeWidget(_iocbox);
}

void SelectDialog::useTransient(bool v) { _useTransient=v; }
