#include "RunStatus.hh"
#include "DamageStats.hh"
#include "QCounter.hh"

#include "pds/xtc/SummaryDg.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/client/L3TIterator.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QPalette>

//#define DBUG

namespace Pds {
  class L3TStats {
  public:
    L3TStats(QGridLayout* layout, unsigned& row) :
      _title_pass(new QLabel("L3TPass")),
      _title_fail(new QLabel("L3TFail")),
      _pass (new QCounter),
      _fail (new QCounter)
    {
      layout->addWidget(_title_pass    ,row,0,Qt::AlignRight);
      layout->addWidget(_pass->widget(),row,1,Qt::AlignLeft);
      row++;
      layout->addWidget(_title_fail    ,row,0,Qt::AlignRight);
      layout->addWidget(_fail->widget(),row,1,Qt::AlignLeft);
      row++;
    }
    ~L3TStats() {
      delete _pass;
      delete _fail;
    }
  public:
    void show(bool v) 
    {
      _title_pass->setVisible(v); 
      _title_fail->setVisible(v); 
      _pass ->widget()->setVisible(v);
      _fail ->widget()->setVisible(v);
    }
    void increment(bool v) 
    {
      if (v) _pass->increment();
      else   _fail->increment();
    }
    void reset() 
    {
      _pass->reset();
      _fail->reset();
    }
    void update()
    {
      _pass->update_count();
      _fail->update_count();
    }
    unsigned long long pass() const { return _pass->get_count(); }
  private:
    QLabel*   _title_pass;
    QLabel*   _title_fail;
    QCounter* _pass;
    QCounter* _fail;
  };
};

using namespace Pds;

RunStatus::RunStatus(QWidget* parent, 
                     PartitionControl& pcontrol,
                     IocControl&       icontrol,
                     PartitionSelect&  partition) :
  QGroupBox("Run Statistics",parent),
  _task    (new Task(TaskObject("runsta"))),
  _duration(new QCounter),
  _events  (new QCounter),
  _damaged (new QCounter),
  _bytes   (new QCounter),
  _pcontrol (pcontrol ),
  _icontrol (icontrol ),
  _partition(partition),
  _details (0),
  _alarm   (false),
  _green          ( new QPalette(Qt::green)),
  _red            ( new QPalette(Qt::red)),
  _run     (0)
{
  _detailsB = new QPushButton("Damage Stats");
  QGridLayout* layout = new QGridLayout(this);
  unsigned row=0;
  layout->addWidget(new QLabel("Duration",this),row,0,Qt::AlignRight);
  layout->addWidget(_duration->widget(),row,1,Qt::AlignLeft); row++;
  layout->addWidget(new QLabel("Events",this),row,0,Qt::AlignRight);
  layout->addWidget(_events->widget(),row,1,Qt::AlignLeft); row++;
  _l3t = new L3TStats(layout,row);
  layout->addWidget(new QLabel("Damaged",this),row,0,Qt::AlignRight);
  layout->addWidget(_damaged->widget(),row,1,Qt::AlignLeft); row++;
  layout->addWidget(new QLabel("Size",this),row,0,Qt::AlignRight);
  layout->addWidget(_bytes->widget(),row,1,Qt::AlignLeft); row++;
  layout->addWidget(_detailsB,row,0,1,2);
  setLayout(layout);

  QObject::connect(this, SIGNAL(changed()), this    , SLOT(update_stats()));
  QObject::connect(this, SIGNAL(reset_s()), this    , SLOT(reset()));
  QObject::connect(this, SIGNAL(damage_alarm_changed(bool)), this    , SLOT(set_damage_alarm(bool)));
  QObject::connect(this, SIGNAL(l3t_used(bool)), this, SLOT(use_l3t(bool)));
  _detailsB->setEnabled(false);

  emit l3t_used(false);
}

RunStatus::~RunStatus()
{
  delete _duration;
  delete _events;
  delete _damaged;
  delete _bytes;
  delete _l3t;
  if (_details) {
    _detailsB->setEnabled(false);
    QObject::disconnect(_detailsB, SIGNAL(clicked()), _details, SLOT(show()));
    QObject::disconnect(this, SIGNAL(changed()), _details, SLOT(update_stats()));
    delete _details;
  }
  delete _green;
  delete _red;
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
  _l3t     ->reset();

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
  _details = new DamageStats(_partition,_pcontrol,_icontrol);

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
    if (_details) {
      _events->increment();
      iterate(&dg->datagram().xtc);
    }
    else {
      printf("RunStatus::events L1Accept and no details\n");
    }
  }
  else if (dg->datagram().seq.service()==TransitionId::Configure) {
    L3TIterator it;
    it.iterate(&dg->datagram().xtc);
    emit l3t_used(it.found());
  }
  return dg; 
}

int RunStatus::process(Xtc* pxtc) {
  const Xtc& xtc = *pxtc;
  if (xtc.contains.id()==TypeId::Id_Xtc) {
    iterate(pxtc);
    return 1;
  }

  if (xtc.contains.value() == SummaryDg::Xtc::typeId().value()) {
    const SummaryDg::Xtc& s = reinterpret_cast<const SummaryDg::Xtc&>(xtc);
#ifdef DBUG
    printf("RunStatus event %08x\n",*reinterpret_cast<uint32_t*>(xtc.payload()));
#endif
    _bytes -> increment(s.payload());
    if (s.damage.value()!=0)
      _damaged->increment();
    _details->increment(s);
    switch(s.l3tresult()) {
    case L1AcceptEnv::Pass:  _l3t->increment(true); break;
    case L1AcceptEnv::Fail:  _l3t->increment(false); break;
    default: break; }
    return 1;
  }

  return 0;
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
  _l3t     ->update();

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

unsigned long long RunStatus::getL3EventNum()
{
  return _l3t->pass();
}

void RunStatus::use_l3t(bool v)
{
  _l3t->show(v);
  updateGeometry();
  resize(minimumWidth(),minimumHeight());
}

int RunStatus::get_counts(unsigned long long *duration, unsigned long long *events, unsigned long long *damaged, unsigned long long *bytes)
{
  if (duration) {
    *duration = _duration->get_count();
  }
  if (events) {
    *events = _events->get_count();
  }
  if (damaged) {
    *damaged = _damaged->get_count();
  }
  if (bytes) {
    *bytes = _bytes->get_count();
  }
  return 0;
}
