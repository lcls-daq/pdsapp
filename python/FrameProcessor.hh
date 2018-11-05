#ifndef PdsApp_Python_FrameProcessor_hh
#define PdsApp_Python_FrameProcessor_hh

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/TransitionId.hh"

namespace Pds {
  class FrameProcessor {
  public:
    virtual ~FrameProcessor() {}
    virtual Dgram* configure(Dgram*) = 0;
    virtual Dgram* event    (Dgram*, const char*) = 0;

    static Dgram* insert(Dgram*              dg,
                         TransitionId::Value tr);
  };
};

#endif
