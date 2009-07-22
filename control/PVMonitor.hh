#ifndef Pds_PVMonitor_hh
#define Pds_PVMonitor_hh

#include "pds/config/ControlConfigType.hh"

#include <list>

namespace Pds {

  class MonitorCA;
  class PVRunnable;

  class PVMonitor {
  public:
    PVMonitor (PVRunnable&);
    virtual ~PVMonitor();
  public:
    bool runnable() const;
  public:
    void configure      (const ControlConfigType&);
    void unconfigure    ();
    void channel_changed();
  private:
    PVRunnable&           _report;
    bool                  _runnable;
    std::list<MonitorCA*> _channels;
  };
};

#endif
