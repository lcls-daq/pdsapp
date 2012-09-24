#include "SelectDialog.hh"

#include "pdsapp/control/NodeSelect.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/collection/PingReply.hh"
#include "pds/utility/StreamPorts.hh"

using namespace Pds;

SelectDialog::SelectDialog(QWidget* parent,
         PartitionControl& control, bool bReadGroupEnable) :
  QDialog  (parent),
  _pcontrol(control),
  _bReadGroupEnable(bReadGroupEnable)
{
  setWindowTitle("Partition Selection");

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(_segbox = new NodeGroup("Readout Nodes",this, _pcontrol.header().platform(), 
    (_bReadGroupEnable? 1:2) ));
  layout->addWidget(_evtbox = new NodeGroup("Processing Nodes",this, _pcontrol.header().platform()));
  layout->addWidget(_rptbox = new NodeGroup("Beamline Data",this, _pcontrol.header().platform()));

  _acceptb = new QPushButton("Ok",this);
  QPushButton* rejectb = new QPushButton("Cancel",this);
  QHBoxLayout* layoutb = new QHBoxLayout;
  layoutb->addWidget(_acceptb);
  layoutb->addWidget(rejectb);
  layout->addLayout(layoutb);
  setLayout(layout);

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
      _segbox->addNode(NodeSelect(hdr, msg));      
      break; 
    }
  case Level::Event   : _evtbox->addNode(NodeSelect(hdr, msg)); break;
  default: break;
  }
}

const QList<Node    >& SelectDialog::selected () const { return _selected; }

const QList<DetInfo >& SelectDialog::detectors() const { return _detinfo; }

const std::set<std::string>& SelectDialog::deviceNames() const { return _deviceNames; }

const QList<ProcInfo>& SelectDialog::segments () const { return _seginfo; }

const QList<BldInfo >& SelectDialog::reporters() const { return _rptinfo; }

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

  accept();
}

void SelectDialog::check_ready()
{
  _acceptb->setEnabled(_segbox->ready() && 
           _evtbox->ready());
}
