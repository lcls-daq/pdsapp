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
  _details (0)
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
  }
  return tr; 
}

void RunStatus::reset()
{
  _duration->reset();
  _events  ->reset();
  _damaged ->reset();
  _bytes   ->reset();
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
}

InDatagram* RunStatus::events     (InDatagram* dg) 
{
  if (dg->datagram().seq.isEvent()) {
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
    if (xtc.damage.value()!=0) {
      _damaged->increment();
      advance += _details->increment(iter,xtc.sizeofPayload()-sizeof(payload));
    }
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
}

Task* RunStatus::task() { return _task; }
unsigned RunStatus::duration() const { return 1000; }
unsigned RunStatus::repetitive() const { return 1; }
