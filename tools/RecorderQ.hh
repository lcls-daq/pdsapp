#ifndef PDS_RECORDERQ
#define PDS_RECORDERQ

#include "pdsapp/tools/Recorder.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {

  class DgSummary;
  class Task;

  class RecorderQ : public Recorder {
  public:
    RecorderQ(const char* fname, unsigned int sliceID, uint64_t chunkSize);
    ~RecorderQ() {}
  public:
    InDatagram* events     (InDatagram* in);
  private:
    Task*      _task;
    Semaphore  _sem;
    DgSummary* _summary;
  };

}
#endif
