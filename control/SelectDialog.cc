#include "SelectDialog.hh"

#include "pdsapp/control/NodeSelect.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/collection/PingReply.hh"

using namespace Pds;

SelectDialog::SelectDialog(QWidget* parent,
			   PartitionControl& control) :
  QDialog  (parent),
  _pcontrol(control) 
{
  setWindowTitle("Partition Selection");

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(_segbox = new NodeGroup("Segment Level",this));
  layout->addWidget(_evtbox = new NodeGroup("Event Level",this));
  layout->addWidget(_recbox = new NodeGroup("Recorder Level",this));

  QPushButton* acceptb = new QPushButton("Ok",this);
  QPushButton* rejectb = new QPushButton("Cancel",this);
  QHBoxLayout* layoutb = new QHBoxLayout;
  layoutb->addWidget(acceptb);
  layoutb->addWidget(rejectb);
  layout->addLayout(layoutb);
  setLayout(layout);

  connect(acceptb, SIGNAL(clicked()), this, SLOT(select()));
  connect(rejectb, SIGNAL(clicked()), this, SLOT(reject()));

  _pcontrol.platform_rollcall(this);
}

SelectDialog::~SelectDialog() 
{
  _pcontrol.platform_rollcall(0);
}

void        SelectDialog::available(const Node& hdr, const PingReply& msg) {
  switch(hdr.level()) {
  case Level::Control : _control = hdr; break;
  case Level::Segment : _segbox->addNode(NodeSelect(hdr, msg)); break;
  case Level::Event   : _evtbox->addNode(NodeSelect(hdr)); break;
  case Level::Recorder: _recbox->addNode(NodeSelect(hdr)); break;
  default: break;
  }
}

const QList<Node>& SelectDialog::selected() const { return _selected; }

QWidget* SelectDialog::display() {
  QWidget* d = new QWidget((QWidget*)0);
  d->setAttribute(Qt::WA_DeleteOnClose,false);
  d->setWindowTitle("Partition Display");
  QVBoxLayout* layout = new QVBoxLayout(d);
  layout->addWidget(_segbox->freeze()); 
  layout->addWidget(_evtbox->freeze()); 
  layout->addWidget(_recbox->freeze()); 
  d->setLayout(layout);
  return d;
}

void SelectDialog::select() {
  _selected.clear();
  _selected << _control;
  _selected << _segbox->selected();
  _selected << _evtbox->selected();
  _selected << _recbox->selected();
  accept();
}
