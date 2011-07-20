#include "PartitionSelect.hh"

#include "pdsapp/control/SelectDialog.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/management/PartitionControl.hh"

#include "pds/collection/Node.hh"

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

  SelectDialog* dialog = new SelectDialog(this, _pcontrol);
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

    _pcontrol.set_partition(_pt_name, _db_path, _nodes, _nnodes, bld_mask);
    _pcontrol.set_target_state(PartitionControl::Configured);

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
