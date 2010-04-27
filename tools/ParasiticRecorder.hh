#ifndef Pds_ParasiticRecorder_hh
#define Pds_ParasiticRecorder_hh

#include "pds/management/EventCallback.hh"

namespace Pds {

  class Task;
  class EventOptions;
  class PartitionMember;

  class ParasiticRecorder: public EventCallback {
  public:
    ParasiticRecorder(Task*         task,
		      EventOptions& options,
		      unsigned      lifetime_sec);
    virtual ~ParasiticRecorder();
  private:
    // Implements EventCallback
    virtual void attached(SetOfStreams& streams);
    virtual void failed(Reason reason);
    virtual void dissolved(const Node& who);

  private:
    Task*          _task;
    EventOptions&  _options;
    Task*          _cleanup_task;
    unsigned       _lifetime_sec;
  };

}

#endif
