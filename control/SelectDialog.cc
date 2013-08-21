#include "SelectDialog.hh"

#include "pdsapp/control/NodeSelect.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/collection/PingReply.hh"
#include "pds/collection/AliasReply.hh"
#include "pds/utility/StreamPorts.hh"

using namespace Pds;

static bool _useTransient = false;

SelectDialog::SelectDialog(QWidget* parent,
         PartitionControl& control, bool bReadGroupEnable) :
  QDialog  (parent),
  _pcontrol(control),
  _bReadGroupEnable(bReadGroupEnable)
{
  setWindowTitle("Partition Selection");

  QGridLayout* layout = new QGridLayout(this);
  layout->addWidget(_segbox = new NodeGroup("Readout Nodes",this, _pcontrol.header().platform(), 
                                            (_bReadGroupEnable? 1:2), _useTransient ), 
                    0, 0);
  layout->addWidget(_evtbox = new NodeGroup("Processing Nodes",this, _pcontrol.header().platform()), 
                    1, 0);
  layout->addWidget(_rptbox = new NodeGroup("Beamline Data",this, _pcontrol.header().platform(),
					    0, _useTransient ),
                    2, 0);

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
}

SelectDialog::~SelectDialog() 
{
  _pcontrol.platform_rollcall(0);
}

void        SelectDialog::available(const Node& hdr, const PingReply& msg) {
  switch(hdr.level()) {
  case Level::Control : _control = hdr; break;
  case Level::Segment : 
    {       
      for(unsigned i=0; i<msg.nsources(); i++) 
      {
        if (msg.source(i).level()==Level::Reporter) 
        {
          const BldInfo& bld = static_cast<const BldInfo&>(msg.source(i));
          Node h(hdr);
          h.fixup(StreamPorts::bld(bld.type()).address(),h.ether());
          _rptbox->addNode(NodeSelect(h,bld));
        }
        ////!!! kludge for adding group
        //else if (msg.source(i).level()==Level::Source) 
        //{
        //   const DetInfo& detInfo = static_cast<const DetInfo&>(msg.source(i));
        //   if (detInfo.devId() >= 100)
        //    ((Node&)hdr).setGroup((int)(detInfo.devId() / 100));
        //}        
      }
      const char *aliasName;
      // TODO consider nsources() > 1
      if ((msg.nsources() > 0) && (aliasName = _pcontrol.lookup_src_alias(msg.source(0)))) {
        char namebuf[SrcAlias::AliasNameMax+1];
        strncpy(namebuf, aliasName, SrcAlias::AliasNameMax);
        namebuf[SrcAlias::AliasNameMax] = '\0';
        _segbox->addNode(NodeSelect(hdr, msg, QString(namebuf)));
      } else {
        _segbox->addNode(NodeSelect(hdr, msg));
      }
      break; 
    }
  case Level::Event   : _evtbox->addNode(NodeSelect(hdr, msg)); break;
  default: break;
  }

  emit changed();
}

void        SelectDialog::aliasCollect(const Node& hdr, const AliasReply& msg)
{
  if (hdr.level() == Level::Segment) {
    int count = (int)msg.naliases();
    for (int ix=0; ix < count; ix++) {
      _pcontrol.add_src_alias(msg.alias(ix));
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
  QGridLayout* layout = static_cast<QGridLayout*>(this->layout());
  if (ns+ne>MaxRows) {
    _clearLayout();
    if (ne+nr > MaxRows) {
      layout->addWidget(_segbox,0,0);
      layout->addWidget(_evtbox,0,1);
      layout->addWidget(_rptbox,0,2);
    }
    else {
      layout->addWidget(_segbox,0,0,2,1);
      layout->addWidget(_evtbox,0,1);
      layout->addWidget(_rptbox,1,1);
    }
  }
  else if (ns+ne+nr > MaxRows) {
    _clearLayout();
    if (ns+ne < ne+nr) {
      layout->addWidget(_segbox,0,0);
      layout->addWidget(_evtbox,1,0);
      layout->addWidget(_rptbox,0,1,2,1);
    }
    else {
      layout->addWidget(_segbox,0,0,2,1);
      layout->addWidget(_evtbox,0,1);
      layout->addWidget(_rptbox,1,1);
    }
  }
  updateGeometry();
}

const QList<Node    >& SelectDialog::selected () const { return _selected; }

const QList<DetInfo >& SelectDialog::detectors() const { return _detinfo; }

const std::set<std::string>& SelectDialog::deviceNames() const { return _deviceNames; }

const QList<ProcInfo>& SelectDialog::segments () const { return _seginfo; }

const QList<BldInfo >& SelectDialog::reporters() const { return _rptinfo; }

const QList<BldInfo >& SelectDialog::transients() const { return _trninfo; }

QWidget* SelectDialog::display() {
  QWidget* d = new QWidget((QWidget*)0);
  d->setAttribute(Qt::WA_DeleteOnClose,false);
  d->setWindowTitle("Partition Display");
  QVBoxLayout* layout = new QVBoxLayout(d);
  layout->addWidget(_segbox->freeze()); 
  layout->addWidget(_evtbox->freeze()); 
  layout->addWidget(_rptbox->freeze()); 
  d->setLayout(layout);
  return d;
}

void SelectDialog::select() {
  _selected.clear();
  _selected << _control;
  _selected << _segbox->selected();
  _selected << _evtbox->selected();
  _rptbox->selected();

  _detinfo << _segbox->detectors();
  _deviceNames = _segbox->deviceNames();
  foreach(Node n, _segbox->selected()) {
    _seginfo  << n.procInfo();
  }

  _rptinfo = _rptbox->reporters();
  _trninfo = _rptbox->transients();

  accept();
}

void SelectDialog::check_ready()
{
  _acceptb->setEnabled(_segbox->ready() && 
           _evtbox->ready());
}

void SelectDialog::_clearLayout()
{
  layout()->removeWidget(_segbox);
  layout()->removeWidget(_evtbox);
  layout()->removeWidget(_rptbox);
}

void SelectDialog::useTransient(bool v) { _useTransient=v; }
