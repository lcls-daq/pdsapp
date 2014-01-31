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
      _rec.record_time(double(SysClk::since(sample))/
                       double(_in->xtc.sizeofPayload()),
                       now);

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

RecorderQ::RecorderQ(const char* fname, unsigned int sliceID, uint64_t chunkSize, bool delay_xfer, bool dont_queue, OfflineClient *offlineclient, const char* expname) :
  Recorder(fname, sliceID, chunkSize, delay_xfer, offlineclient, expname),
  _task(new Task(TaskObject("RecEvt"))),
  _sem (Semaphore::EMPTY),
  _dont_queue(dont_queue)
{
  MonGroup* group = new MonGroup("RecQ");
  VmonServerManager::instance()->cds().add(group);

  { const int t_bins = 64;
    const float lt1 = 10.; // 100MB/s
    MonDescTH1F rec_time("Rec Time", "[ms/MB]", "",
                         t_bins, 0., lt1);
    _rec_time = new MonEntryTH1F(rec_time);
    group->add(_rec_time); }

  { const int logt_bins = 64;
    const float lt0 = -1.; // 10GB/s
    const float lt1 =  2.; // 10MB/s
    MonDescTH1F rec_time("Log Rec Time", "log10 [ms/MB]", "",
                         logt_bins, lt0, lt1);
    _rec_time_log = new MonEntryTH1F(rec_time);
    group->add(_rec_time_log); }
}

InDatagram* RecorderQ::events(InDatagram* in) 
{
  if (in->datagram().seq.service()==TransitionId::L1Accept) {
    if (_busy)
      return in;
    _busy = _dont_queue;
    _task->call(new QueuedAction(in,*this));
  }
  else {
    _task->call(new QueuedAction(in,*this,&_sem));
    _sem.take();
  }
  return (InDatagram*)Appliance::DontDelete;
}

void RecorderQ::record_time(double t_ns_byte, const ClockTime& now)
{
  _rec_time    ->addcontent(1., t_ns_byte);
  _rec_time_log->addcontent(1., log10f(t_ns_byte));
  _rec_time    ->time(now);
  _rec_time_log->time(now);
  _busy = false;
}
