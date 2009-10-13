#include "RunStatus.hh"

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/xtc/CDatagramIterator.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

using namespace Pds;

class QCounter {
public:
  QCounter() : _widget(new QLabel(0)), _count(0) {}
  ~QCounter() {}
public:
  QWidget* widget() const { return _widget; }
public:
  void reset    () { _count=0; }
  void increment() { _count++; }
  void increment(unsigned n) { _count+=n; }
  void update_bytes() { 
    unsigned long long c = _count;
    QString unit;
    if      (c < 10000ULL   )    { unit=QString("bytes"); }
    else if (c < 10000000ULL)    { c /= 1000ULL; unit=QString("kBytes"); }
    else if (c < 10000000000ULL) { c /= 1000000ULL; unit=QString("MBytes"); }
    else                         { c /= 1000000000ULL; unit=QString("GBytes"); }
    _widget->setText(QString("%1 %2").arg(c).arg(unit)); 
  }
  void update_count() { _widget->setText(QString::number(_count)); }
  void update_time () { _widget->setText(QString("%1:%2:%3").arg(_count/3600).arg((_count%3600)/60).arg(_count%60)); }
private:
  QLabel* _widget;
  unsigned long long _count;
};
  

RunStatus::RunStatus(QWidget* parent) :
  QGroupBox("Run Statistics",parent),
  _task    (new Task(TaskObject("runsta"))),
  _pool    (sizeof(CDatagramIterator),1),
  _duration(new QCounter),
  _events  (new QCounter),
  _damaged (new QCounter),
  _bytes   (new QCounter)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->addWidget(new QLabel("Duration",this),0,0,Qt::AlignRight);
  layout->addWidget(_duration->widget(),0,1,Qt::AlignLeft);
  layout->addWidget(new QLabel("Events",this),1,0,Qt::AlignRight);
  layout->addWidget(_events->widget(),1,1,Qt::AlignLeft);
  layout->addWidget(new QLabel("Damaged",this),2,0,Qt::AlignRight);
  layout->addWidget(_damaged->widget(),2,1,Qt::AlignLeft);
  layout->addWidget(new QLabel("Size",this),3,0,Qt::AlignRight);
  layout->addWidget(_bytes->widget(),3,1,Qt::AlignLeft);
  setLayout(layout);
}

RunStatus::~RunStatus()
{
  delete _duration;
  delete _events;
  delete _damaged;
  delete _bytes;
}

Transition* RunStatus::transitions(Transition* tr) 
{
  if (tr->id() == TransitionId::BeginRun) {
    _duration->reset(); _duration->update_time();
    _events  ->reset(); _events  ->update_count();
    _damaged ->reset(); _damaged ->update_count();
    _bytes   ->reset(); _bytes   ->update_bytes();
    start();
  }
  else if (tr->id() == TransitionId::EndRun) {
    cancel();
  }
  return tr; 
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
    if (xtc.damage.value()!=0)
      _damaged->increment();
  }
  return advance;
}

void  RunStatus::expired() 
{
  _duration->increment();
  _duration->update_time();
  _events  ->update_count();
  _damaged ->update_count();
  _bytes   ->update_bytes();
}

Task* RunStatus::task() { return _task; }
unsigned RunStatus::duration() const { return 1000; }
unsigned RunStatus::repetitive() const { return 1; }
