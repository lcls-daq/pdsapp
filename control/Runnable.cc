#include "Runnable.hh"

#include "pds/management/PartitionControl.hh"

#include <QtGui/QLabel>
#include <QtGui/QGridLayout>

using namespace Pds;


static const char* _Map           = "ALLOCATE";
static const char* _Begin_Running = "BEGIN RUNNING";
static const char* _End_Running   = "END RUNNING";
static const char* _Pause         = "PAUSE";
static const char* _Resume        = "RESUME";
static const char* _Shutdown      = "SHUTDOWN";
static const char* _NoChange      = "NO CHANGE";

Runnable::Runnable(QWidget* parent,
		   PartitionControl& control) :
  QGroupBox("Detector",parent),
  _control (control)
{
  _monitor_display = new QLabel("-",this);
  _monitor_display->setAlignment(Qt::AlignHCenter);
  _control_display = new QLabel("-",this);
  _control_display->setAlignment(Qt::AlignHCenter);

  QGridLayout* layout = new QGridLayout(this);
  layout->addWidget(new QLabel("Monitor State",this),0,0,1,1,Qt::AlignHCenter);
  layout->addWidget(_monitor_display,1,0,1,1);
  layout->addWidget(new QLabel("Control State",this),0,1,1,1,Qt::AlignHCenter);
  layout->addWidget(_control_display,1,1,1,1);
  setLayout(layout);

  QObject::connect(this, SIGNAL(monitor_changed(const QString&)),
		   this, SLOT  (update_monitor(const QString&)));
}

Runnable::~Runnable()
{
}

void Runnable::update_monitor(const QString& state)
{
  _monitor_display->setText(state);
  if (state==_Pause)
    _control.pause();
  else if (state==_Resume)
    _control.resume(); 
  else if (state==_Begin_Running) 
    _control.set_target_state(PartitionControl::Enabled);
  else if (state==_End_Running)
    _control.set_target_state(PartitionControl::Configured);
  else if (state==_Map)
    _control.set_target_state(PartitionControl::Mapped);
  else if (state==_Shutdown)
    _control.set_target_state(PartitionControl::Unmapped);
}

Transition* Runnable::transitions(Transition* tr) 
{
  QString label(TransitionId::name(tr->id()));
  _display->setText(label);

  emit state_changed(); 
  return tr; 
}
InDatagram* Runnable::occurrences(InDatagram* dg) { return dg; }
InDatagram* Runnable::events     (InDatagram* dg) { return dg; }
