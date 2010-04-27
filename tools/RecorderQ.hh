#ifndef PDS_RECORDERQ
#define PDS_RECORDERQ

#include "pdsapp/tools/Recorder.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {

  class DgSummary;
  class Task;
  class MonEntryTH1F;
  class ClockTime;

  class RecorderQ : public Recorder {
  public:
    RecorderQ(const char* fname, unsigned int sliceID, uint64_t chunkSize, 
	      bool dont_queue=false);
    ~RecorderQ() {}
  public:
    InDatagram* events     (InDatagram* in);
    void        record_time(unsigned, const ClockTime&);
  private:
    Task*      _task;
    Semaphore  _sem;
    bool       _dont_queue;  // queue depth is one
    bool       _busy;
    MonEntryTH1F* _rec_time;
  };

}
#endif
