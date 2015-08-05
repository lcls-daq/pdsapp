#ifndef PDS_RECORDERQ
#define PDS_RECORDERQ

#include "pdsapp/tools/Recorder.hh"
#include "pdsapp/tools/DgSummary.hh"
#include "pds/service/Semaphore.hh"
#include "pds/offlineclient/OfflineClient.hh"

namespace Pds {

  class DgSummary;
  class Task;
  class MonEntryTH1F;
  class ClockTime;

  class RecorderQ : public Recorder {
  public:
     RecorderQ(const char* fname, unsigned int sliceID, uint64_t chunkSize, 
               unsigned uSizeThreshold,
               bool delay_xfer=false,
               bool dont_queue=false,
               OfflineClient *offlineclient=NULL,
               const char* expname=NULL);
    ~RecorderQ() {}
  public:
    Transition* transitions(Transition* in);
    InDatagram* events     (InDatagram* in);
    void        record_time(double, const ClockTime&);
  private:
    Task*      _task;
    Semaphore  _sem;
    bool       _dont_queue;  // queue depth is one
    bool       _busy;
    MonEntryTH1F* _rec_time;
    MonEntryTH1F* _rec_time_log;
    DgSummary  _summary;
  };

}
#endif
