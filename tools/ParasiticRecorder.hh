#ifndef Pds_ParasiticRecorder_hh
#define Pds_ParasiticRecorder_hh

#include "pds/management/EventCallback.hh"
#include "pds/offlineclient/OfflineClient.hh"

namespace Pds {

  class Task;
  class EventOptions;
  class PartitionMember;

  class ParasiticRecorder: public EventCallback {
  public:
    ParasiticRecorder(Task*         task,
		      EventOptions& options,
		      unsigned      lifetime_sec,
		      const char *  partition,
		      const char *  offlinerc);
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
    const char *   _partition;
    const char *   _offlinerc;
    const char *   _expname;
    OfflineClient* _offlineclient;
  };

}

#endif
