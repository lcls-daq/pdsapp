#ifndef Pds_PVManager_hh
#define Pds_PVManager_hh

#include "pdsapp/control/PVRunnable.hh"
#include "pds/config/ControlConfigType.hh"

namespace Pds {
  class PVMonitor;
  class PVControl;
  class PVManager : public PVRunnable {
  public:
    PVManager(PVRunnable&);
    ~PVManager();
  public:
    void runnable_change(bool);
  public:
    void configure      (const ControlConfigType&);
    void unconfigure    ();
  private:
    PVRunnable& _pvrunnable;
    PVMonitor*  _pvmonitor;
    PVControl*  _pvcontrol;
  };
};

#endif
