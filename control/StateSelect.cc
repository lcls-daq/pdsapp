#include "pdsapp/control/StateSelect.hh"
#include "pdsapp/control/Preferences.hh"

#include "pds/management/PartitionControl.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>
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
static const char* YES_STR = "DO_RECORD";
static const char* NO_STR  = "DO_NOT_RECORD";

StateSelect::StateSelect(QWidget* parent,
			 PartitionControl& control) :
  QGroupBox("Control",parent),
  _control (control),
  _green   (new QPalette(Qt::green)),
  _yellow  (new QPalette(Qt::yellow)),
  _sem     (Semaphore::EMPTY)
{
  _display = new QLabel("-",this);
  _display->setAutoFillBackground(true);
  _display->setAlignment(Qt::AlignHCenter);
  _display->setPalette  (*_yellow);

  QGridLayout* layout = new QGridLayout(this);
  layout->addWidget(_record = new QCheckBox("Record Run"), 0,0,1,2,Qt::AlignHCenter);
  layout->addWidget(new QLabel("Target State",this),1,0,1,1,Qt::AlignHCenter);
  layout->addWidget(_select = new QComboBox(this),2,0,1,1);
  layout->addWidget(new QLabel("Last Transition",this),1,1,1,1,Qt::AlignHCenter);
  layout->addWidget(_display,2,1,1,1);
  setLayout(layout);

  QFont font = _record->font();
  font.setPointSize(font.pointSize() + 4);
  _record->setFont(font);
  _record->setAutoFillBackground(true);

  bool do_record=false;
  {
    QList<QString> t;
    Preferences pref(qPrintable(title()), _control.header().platform(),"r");
    pref.read(t);
    do_record = (t.size()>0 && t[0]==YES_STR);
  }
  _record->setEnabled(true);
  _record->setChecked(do_record);
  set_record(do_record);

  QObject::connect(_record, SIGNAL(toggled(bool)), 
		   this, SLOT(set_record(bool)));
  QObject::connect(this, SIGNAL(remote_record(bool)),
                   _record, SLOT(setChecked(bool)));
  QObject::connect(this, SIGNAL(state_changed(QString)),
		   this, SLOT(populate(QString)));
  QObject::connect(_select, SIGNAL(activated(const QString&)), 
		   this, SLOT(selected(const QString&)));
  QObject::connect(this, SIGNAL(_enable_control(bool)),
		   _select, SLOT(setEnabled(bool)));
  QObject::connect(this, SIGNAL(_qtsync()), this, SLOT(unblock()));
		   
}

StateSelect::~StateSelect()
{
  delete _green;
  delete _yellow;
}

bool StateSelect::control_enabled() const { return _select->isEnabled(); }
void StateSelect::enable_control () { emit _enable_control(true); }
void StateSelect::disable_control() { emit _enable_control(false); }
bool StateSelect::record_state() const { return _record->isChecked(); }
void StateSelect::set_record_state(bool r) { emit remote_record(r); qtsync(); }

void StateSelect::populate(QString label)
{
  _display->setText(label);

  QObject::disconnect(_select, SIGNAL(activated(const QString&)), 
		      this, SLOT(selected(const QString&)));
  _select->clear();
  switch(_control.target_state()) {
  case PartitionControl::Unmapped:   
                                       _select->addItem(_Allocate);
				       _select->addItem(_Begin_Running);
				       emit configured(false);
				       _record->setEnabled(true);
				       break;
  case PartitionControl::Mapped:
                                       emit configured(false);
				       // _record->setEnabled(true);
				       break;
  case PartitionControl::Configured:
                                       _select->addItem(_Begin_Running);
				       _select->addItem(_Shutdown);
				       emit configured(true);
				       _record->setEnabled(true);
				       break;
  case PartitionControl::Running :
                                       _select->addItem(_End_Running);
				       _select->addItem(_Shutdown);
				       emit configured(true);
				       _record->setEnabled(false);
				       break;
  case PartitionControl::Disabled:
                                      _select->addItem(_Enable);
                                      _select->addItem(_End_Running);
				      _select->addItem(_Shutdown);
                                      emit configured(true);
				      break;
  case PartitionControl::Enabled :
                                       _select->addItem(_Disable);
				       _select->addItem(_Next_Cycle);
				       _select->addItem(_End_Running);
				       _select->addItem(_Shutdown);
				       emit configured(true);
				       _record->setEnabled(false);
				       break;
  case PartitionControl::NumberOfStates: break;
  }
  QObject::connect(_select, SIGNAL(activated(const QString&)), 
		   this, SLOT(selected(const QString&)));

  _display->setPalette( _control.target_state()==PartitionControl::Enabled ? *_green : *_yellow );
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

void StateSelect::set_record(bool r)
{
  _record->setPalette(r ? *_green : *_yellow);
  _control.use_run_info(r);

  Preferences pref(qPrintable(title()), _control.header().platform(), "w");
  pref.write(r ? YES_STR:NO_STR);
}

Transition* StateSelect::transitions(Transition* tr) 
{
  if (tr->id() == TransitionId::Unmap) {
    QString label(TransitionId::name(tr->id()));
    emit state_changed(label); 
 }
  return tr; 
}

InDatagram* StateSelect::events     (InDatagram* dg) 
{
  if (dg->datagram().seq.type()==Sequence::Event &&
      dg->datagram().seq.service()!=TransitionId::L1Accept) {
    QString label(TransitionId::name(dg->datagram().seq.service()));
    emit state_changed(label); 
  }
  return dg; 
}

void StateSelect::qtsync() { emit _qtsync(); _sem.take(); }

void StateSelect::unblock() { _sem.give(); }
