#ifndef PDS_RECORDER
#define PDS_RECORDER

#include "pds/utility/Appliance.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {

  class Task;

  class Recorder : public Appliance {
  public:
    Recorder(const char* fname);
    ~Recorder() {}
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
