#ifndef SimCam_hh
#define SimCam_hh

#include "pds/utility/Appliance.hh"
#include "pds/service/Semaphore.hh"
#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {
  class SimCam : public Appliance {
  public:
    SimCam();
    ~SimCam();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
    Occurrence* occurrences(Occurrence*);
  private:
    class MyIter : public XtcIterator {
    public:
      MyIter() {}
      int process(Xtc*);
    };
  };
};

#endif
