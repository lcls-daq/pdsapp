#ifndef Pds_PVRunnable_hh
#define Pds_PVRunnable_hh

namespace Pds {

  class PVRunnable {
  public:
    virtual ~PVRunnable() {}
    virtual void runnable_change(bool) = 0;
  };

};

#endif
