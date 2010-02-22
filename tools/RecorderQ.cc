#include "RecorderQ.hh"
#include "pdsapp/tools/DgSummary.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"

namespace Pds {

  class QueuedAction : public Routine {
  public:
    QueuedAction(InDatagram* in, Recorder& rec, Semaphore* sem = 0) :
      _in(in), _rec(rec), _sem(sem) {}
    ~QueuedAction() {}
  public:
    void routine() {
      InDatagram* o = _rec.Recorder::events(_in);
      if (_sem)	_sem->give();
      else	delete o;
      delete this;
    }
  private:
    InDatagram* _in;
    Recorder&   _rec;
    Semaphore*  _sem;
  };
};

using namespace Pds;

RecorderQ::RecorderQ(const char* fname, unsigned int sliceID, uint64_t chunkSize) :
  Recorder(fname, sliceID, chunkSize),
  _task(new Task(TaskObject("RecEvt"))),
  _sem (Semaphore::EMPTY),
  _summary(new DgSummary)
{
}

InDatagram* RecorderQ::events(InDatagram* in) 
{
  if (in->datagram().seq.service()==TransitionId::L1Accept) {
    post(_summary->events(in));
    _task->call(new QueuedAction(in,*this));
    return (InDatagram*)Appliance::DontDelete;
  }
  else {
    _task->call(new QueuedAction(in,*this,&_sem));
    _sem.take();
    return in;
  }
}
