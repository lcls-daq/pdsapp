#include "RunStatus.hh"
#include "DamageStats.hh"
#include "QCounter.hh"

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/xtc/CDatagramIterator.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QPushButton>
#include <QtGui/QPalette>

using namespace Pds;

RunStatus::RunStatus(QWidget* parent, PartitionSelect& partition) :
  QGroupBox("Run Statistics",parent),
  _task    (new Task(TaskObject("runsta"))),
  _pool    (sizeof(CDatagramIterator),1),
  _duration(new QCounter),
  _events  (new QCounter),
  _damaged (new QCounter),
  _bytes   (new QCounter),
  _partition(partition),
  _details (0),
  _alarm   (false),
  _green          ( new QPalette(Qt::green)),
  _red            ( new QPalette(Qt::red))
{
  _detailsB = new QPushButton("Damage Stats");
  QGridLayout* layout = new QGridLayout(this);
  layout->addWidget(new QLabel("Duration",this),0,0,Qt::AlignRight);
  layout->addWidget(_duration->widget(),0,1,Qt::AlignLeft);
  layout->addWidget(new QLabel("Events",this),1,0,Qt::AlignRight);
  layout->addWidget(_events->widget(),1,1,Qt::AlignLeft);
  layout->addWidget(new QLabel("Damaged",this),2,0,Qt::AlignRight);
  layout->addWidget(_damaged->widget(),2,1,Qt::AlignLeft);
  layout->addWidget(new QLabel("Size",this),3,0,Qt::AlignRight);
  layout->addWidget(_bytes->widget(),3,1,Qt::AlignLeft);
  layout->addWidget(_detailsB,4,0,1,2);
  setLayout(layout);

  QObject::connect(this, SIGNAL(changed()), this    , SLOT(update_stats()));
  QObject::connect(this, SIGNAL(reset_s()), this    , SLOT(reset()));
  QObject::connect(this, SIGNAL(damage_alarm_changed(bool)), this    , SLOT(set_damage_alarm(bool)));

  _detailsB->setEnabled(false);
}

RunStatus::~RunStatus()
{
  delete _duration;
  delete _events;
  delete _damaged;
  delete _bytes;
  if (_details) {
    _detailsB->setEnabled(false);
    QObject::disconnect(_detailsB, SIGNAL(clicked()), _details, SLOT(show()));
    QObject::disconnect(this, SIGNAL(changed()), _details, SLOT(update_stats()));
    delete _details;
  }
}

Transition* RunStatus::transitions(Transition* tr) 
{
  if (tr->id() == TransitionId::BeginRun) {
    emit reset_s();
    start();
  }
  else if (tr->id() == TransitionId::EndRun) {
    cancel();
    emit changed();
  }
  else if (tr->id() == TransitionId::Disable) {
    unsigned d = unsigned(_duration->value());
    printf("Duration: %02d:%02d:%02d\n", d/3600, (d%3600)/60, d%60);
    printf("Events  : %lld\n",_events ->value());
    printf("Damaged : %lld\n",_damaged->value());
    printf("Bytes   : %lld\n",_bytes  ->value());
    _details->dump();
  }
  return tr; 
}

void RunStatus::reset()
{
  _duration->reset();
  _events  ->reset();
  _damaged ->reset();
  _bytes   ->reset();

  _prev_events  = 0;
  _prev_damaged = 0;
  
  update_stats();

  if (_details) {
    QObject::disconnect(_detailsB, SIGNAL(clicked()), _details, SLOT(show()));
    QObject::disconnect(this, SIGNAL(changed()), _details, SLOT(update_stats()));
    delete _details;
  }
  else {
    _detailsB->setEnabled(true);
  }
  _details = new DamageStats(_partition);
  QObject::connect(_detailsB, SIGNAL(clicked()), _details, SLOT(show()));
  QObject::connect(this, SIGNAL(changed()), _details, SLOT(update_stats()));

  _alarm = false;
  emit damage_alarm_changed(false);
}

InDatagram* RunStatus::events     (InDatagram* dg) 
{
  if (dg->datagram().seq.isEvent()) {
    /*
    { unsigned* d = reinterpret_cast<unsigned*>(dg->datagram().xtc.payload());
      unsigned* e = d + (dg->datagram().xtc.sizeofPayload()>>2);
      printf("RS::events payload: ");
      while(d<e) printf(" %08x",*d++);
      printf("\n");
    }
    */
    if (!_details) {
      printf("RunStatus::events L1Accept and no details\n");
      return 0;
    }

    _events->increment();

    InDatagramIterator* iter = dg->iterator(&_pool);
    iterate(dg->datagram().xtc,iter);
    delete iter;

    return 0;
  }
  return dg; 
}

int RunStatus::process(const Xtc& xtc, InDatagramIterator* iter) {

  if (xtc.contains.id()==TypeId::Id_Xtc)
    return iterate(xtc,iter);

  int advance=0;
  if (xtc.contains.id()==TypeId::Any) {
    unsigned payload;
    advance = iter->copy(&payload, sizeof(payload));
    _bytes -> increment(payload);
    if (xtc.damage.value()!=0)
      _damaged->increment();
    advance += _details->increment(iter,xtc.sizeofPayload()-sizeof(payload));
  }
  return advance;
}

void  RunStatus::expired() 
{
  _duration->increment();
  emit changed();
}

void RunStatus::update_stats()
{
  _duration->update_time();
  _events  ->update_count();
  _damaged ->update_count();
  _bytes   ->update_bytes();

  const double DamageAlarmFraction = 0.2;
  unsigned uevents  = _events ->get_count();
  unsigned udamaged = _damaged->get_count();
  unsigned uthresh  = unsigned(double(uevents)*DamageAlarmFraction) + 20;
  double events   = uevents  - _prev_events;
  double damaged  = udamaged - _prev_damaged;
  double thresh = events*DamageAlarmFraction;
  bool alarm = (damaged > thresh) || (udamaged > uthresh);
  if (alarm && !_alarm)
    emit damage_alarm_changed(true);
  else if (!alarm && _alarm)
    emit damage_alarm_changed(false);

  _alarm = alarm;
  _prev_events  = uevents;
  _prev_damaged = udamaged;
}

Task* RunStatus::task() { return _task; }
unsigned RunStatus::duration() const { return 1000; }
unsigned RunStatus::repetitive() const { return 1; }

void RunStatus::set_damage_alarm(bool alarm)
{ _detailsB->setPalette(alarm ? *_red : *_green); }

unsigned long long RunStatus::getEventNum()
{
  return _events->get_count();
}
