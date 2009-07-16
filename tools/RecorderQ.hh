#ifndef PDS_RECORDERQ
#define PDS_RECORDERQ

#include "pds/utility/Appliance.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {

  class Task;

  class RecorderQ : public Appliance {
  public:
    RecorderQ(const char* fname);
    ~RecorderQ() {}
    Transition* transitions(Transition*);
    InDatagram* occurrences(InDatagram* in);
    InDatagram* events     (InDatagram* in);

  private:
    FILE* _f;
    Pool* _pool;
    Task* _task;
    Semaphore _sem;
  };

}
#endif
