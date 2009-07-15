
#include <stdio.h>

#include "Recorder.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"

namespace Pds {

  class REvent {
  public:
    REvent(InDatagram* in, FILE* f, Pool* pool) : _in(in), _f(f), _pool(pool) {}
    ~REvent() {}
  public:
    void write()
    {
      //  write the event header
      const Datagram& dg = _in->datagram();
      fwrite(&dg,sizeof(dg),1,_f);

      //  write the payload
      InDatagramIterator& iter = *_in->iterator(_pool);
      struct iovec iov;
      int remaining = dg.xtc.sizeofPayload();
      while(remaining) {
	int isize = iter.read(&iov,1,remaining);
	printf("Iterator found %x bytes at %p dg at %p next %p\n",
	       iov.iov_len,iov.iov_base,&dg,
	       dg.xtc.next());
	int fsize = fwrite(iov.iov_base,iov.iov_len,1,_f);
	if (fsize != isize)
	  printf("fwrite size %d/%d bytes\n",fsize,isize);
	remaining -= isize;
      }
      fflush(_f);
      delete &iter;
    }
  public:
    InDatagram* in() { return _in; }
  private:
    InDatagram* _in;
    FILE*       _f;
    Pool*       _pool;
  };

  class QueuedWrite : public Routine {
  public:
    QueuedWrite(const REvent& evt, Semaphore* sem = 0) :
      _evt(evt), _sem(sem) {}
    ~QueuedWrite() {}
  public:
    void routine() {
      _evt.write();
      if (_sem)	_sem->give();
      else	delete _evt.in();
      delete this;
    }
  private:
    REvent     _evt;
    Semaphore* _sem;
  };
};

using namespace Pds;

Recorder::Recorder(const char* fname) : 
  Appliance(), 
  _pool(new GenericPool(sizeof(ZcpDatagramIterator),1)),
  _task(new Task(TaskObject("RecEvt"))),
  _sem (Semaphore::EMPTY)
{
  printf("Opening file: %s\n",fname);
  _f=fopen(fname,"w");
}

InDatagram* Recorder::events(InDatagram* in) {
  REvent evt(in, _f, _pool);
  if (in->datagram().seq.service()==TransitionId::L1Accept) {
    _task->call(new QueuedWrite(evt));
    return (InDatagram*)Appliance::DontDelete;
  }
  else {
    _task->call(new QueuedWrite(evt,&_sem));
    _sem.take();
    return in;
  }
}

Transition* Recorder::transitions(Transition* tr) {
  printf("Received transition\n");
  return tr;
}

InDatagram* Recorder::occurrences(InDatagram* in) {
  return in;
}
