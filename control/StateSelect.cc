#include "StateSelect.hh"

#include "pds/management/PartitionControl.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPalette>

using namespace Pds;


static const char* _Allocate      = "ALLOCATE";
static const char* _Begin_Running = "BEGIN RUNNING";
static const char* _Disable       = "DISABLE";
static const char* _Enable        = "ENABLE";
static const char* _Next_Cycle    = "NEXT CYCLE";
static const char* _End_Running   = "END RUNNING";
static const char* _Shutdown      = "SHUTDOWN";
static const char* _NoChange      = "NO CHANGE";

StateSelect::StateSelect(QWidget* parent,
			 PartitionControl& control) :
  QGroupBox("Control",parent),
  _control (control),
  _green   (new QPalette(Qt::green)),
  _yellow  (new QPalette(Qt::yellow))
{
  _display = new QLabel("-",this);
  _display->setAlignment(Qt::AlignHCenter);
  _display->setPalette  (*_green);

  QGridLayout* layout = new QGridLayout(this);
  layout->addWidget(new QLabel("Target State",this),0,0,1,1,Qt::AlignHCenter);
  layout->addWidget(_select = new QComboBox(this),1,0,1,1);
  layout->addWidget(new QLabel("Last Transition",this),0,1,1,1,Qt::AlignHCenter);
  layout->addWidget(_display,1,1,1,1);
  setLayout(layout);

  QObject::connect(this, SIGNAL(state_changed()),
		   this, SLOT(populate()));
  QObject::connect(_select, SIGNAL(activated(const QString&)), 
		   this, SLOT(selected(const QString&)));
}

StateSelect::~StateSelect()
{
  delete _green;
  delete _yellow;
}

void StateSelect::populate()
{
  QObject::disconnect(_select, SIGNAL(activated(const QString&)), 
		      this, SLOT(selected(const QString&)));
  _select->clear();
  switch(_control.target_state()) {
  case PartitionControl::Unmapped:   
                                       _select->addItem(_Allocate);
				       _select->addItem(_Begin_Running);
				       emit deallocated();
				       break;
  case PartitionControl::Mapped:
                                       emit allocated();
				       break;
  case PartitionControl::Configured:
                                       _select->addItem(_Begin_Running);
				       _select->addItem(_Shutdown);
				       break;
  case PartitionControl::Running :
                                       _select->addItem(_End_Running);
				       _select->addItem(_Shutdown);
				       break;
  case PartitionControl::Disabled:
                                      _select->addItem(_Enable);
                                      _select->addItem(_End_Running);
				      _select->addItem(_Shutdown);
				      break;
  case PartitionControl::Enabled :
                                       _select->addItem(_Disable);
				       _select->addItem(_Next_Cycle);
				       _select->addItem(_End_Running);
				       _select->addItem(_Shutdown);
				       break;
  case PartitionControl::NumberOfStates: break;
  }
  _select->addItem(_NoChange);
  QObject::connect(_select, SIGNAL(activated(const QString&)), 
		   this, SLOT(selected(const QString&)));

  _display->setPalette( _control.current_state()==_control.target_state() ? *_green : *_yellow );
}

void StateSelect::selected(const QString& state)
{
  if (state==_Disable)
    _control.set_target_state(PartitionControl::Disabled);
  else if (state==_Enable)
    _control.set_target_state(PartitionControl::Enabled);
  else if (state==_Begin_Running) 
    _control.set_target_state(PartitionControl::Enabled);
  else if (state==_End_Running)
    _control.set_target_state(PartitionControl::Configured);
  else if (state==_Allocate)
    _control.set_target_state(PartitionControl::Configured);
  else if (state==_Shutdown)
    _control.set_target_state(PartitionControl::Unmapped);
  else if (state==_Next_Cycle)
    _control.message(_control.header(),Occurrence(OccurrenceId::SequencerDone));
}

Transition* StateSelect::transitions(Transition* tr) 
{
  if (tr->id() == TransitionId::Unmap) {
    QString label(TransitionId::name(tr->id()));
    _display->setText(label);

    emit state_changed(); 
 }
  return tr; 
}

InDatagram* StateSelect::events     (InDatagram* dg) 
{
  if (dg->datagram().seq.type()==Sequence::Event) {
    QString label(TransitionId::name(dg->datagram().seq.service()));
    _display->setText(label);

    emit state_changed(); 
  }
  return dg; 
}
