#ifndef PDS_RECORDERQ
#define PDS_RECORDERQ

#include "pdsapp/tools/Recorder.hh"
#include "pds/service/Semaphore.hh"
#include "pds/offlineclient/OfflineClient.hh"

namespace Pds {

  class Task;
  class MonEntryTH1F;
  class ClockTime;

  class RecorderQ : public Recorder {
  public:
     RecorderQ(const char* fname, unsigned int sliceID, uint64_t chunkSize, 
               unsigned uSizeThreshold,
               bool delay_xfer=false,
               OfflineClient *offlineclient=NULL,
               const char* expname=NULL);
    ~RecorderQ() {}
  public:
    InDatagram* events     (InDatagram* in);
    void        record_time(double, const ClockTime&);
  private:
    Task*      _task;
    Semaphore  _sem;
    MonEntryTH1F* _rec_time;
    MonEntryTH1F* _rec_time_log;
  };

}
#endif
