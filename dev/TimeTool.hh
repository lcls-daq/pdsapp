#ifndef TimeTool_hh
#define TimeTool_hh

#include "pds/utility/Appliance.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {
  class FexApp;
  class Task;
  class TimeTool : public Appliance {
  public:
    TimeTool(const char* fname);
    ~TimeTool();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
    Occurrence* occurrences(Occurrence*);
  private:
    Task*     _task;
    FexApp*   _fex;
    Semaphore _sem;
    uint32_t  _bykik;
    uint32_t  _no_laser;
  };
};

#endif
