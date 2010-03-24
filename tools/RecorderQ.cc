#include "RecorderQ.hh"
#include "pdsapp/tools/DgSummary.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"

#include "pds/vmon/VmonServerManager.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/service/SysClk.hh"
#include <math.h>

namespace Pds {

  class QueuedAction : public Routine {
  public:
    QueuedAction(InDatagram* in, RecorderQ& rec, Semaphore* sem = 0) :
      _in(in), _rec(rec), _sem(sem) {}
    ~QueuedAction() {}
  public:
    void routine() {

      ClockTime now(_in->datagram().seq.clock());
      unsigned sample = SysClk::sample();
      _rec.post(_rec.Recorder::events(_in));
      _rec.record_time(SysClk::since(sample),now);

      if (_sem)	_sem->give();
      delete this;
    }
  private:
    InDatagram* _in;
    RecorderQ&  _rec;
    Semaphore*  _sem;
  };
};

using namespace Pds;

RecorderQ::RecorderQ(const char* fname, unsigned int sliceID, uint64_t chunkSize) :
  Recorder(fname, sliceID, chunkSize),
  _task(new Task(TaskObject("RecEvt"))),
  _sem (Semaphore::EMPTY)
{
  MonGroup* group = new MonGroup("RecQ");
  VmonServerManager::instance()->cds().add(group);

  const int logt_bins = 64;
  const float lt0 = 6.; // 1ms
  const float lt1 = 9.; // 1s
  MonDescTH1F rec_time("Log Rec Time", "log10 [ns]", "",
		       logt_bins, lt0, lt1);
  _rec_time = new MonEntryTH1F(rec_time);
  group->add(_rec_time);
}

InDatagram* RecorderQ::events(InDatagram* in) 
{
  if (in->datagram().seq.service()==TransitionId::L1Accept) {
    _task->call(new QueuedAction(in,*this));
  }
  else {
    _task->call(new QueuedAction(in,*this,&_sem));
    _sem.take();
  }
  return (InDatagram*)Appliance::DontDelete;
}

void RecorderQ::record_time(unsigned t, const ClockTime& now)
{
  _rec_time->addcontent(1., log10f(double(t)));
  _rec_time->time(now);
}
