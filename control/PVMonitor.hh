#ifndef Pds_PVMonitor_hh
#define Pds_PVMonitor_hh

#include "pds/config/ControlConfigType.hh"

#include <list>

namespace Pds {

  class MonitorCA;

  class PVMonitor {
  public:
    enum State { OK, NotOK };
  public:
    PVMonitor ();
    virtual ~PVMonitor();
  public:
    State state() const;
  public:
    void configure(const ControlConfigType&);
    void channel_changed();
  public:
    const std::list<MonitorCA*> channels() const { return _channels; }
  private:
    virtual void state_changed(State) = 0;
  private:
    State                 _state;
    std::list<MonitorCA*> _channels;
  };
};

#endif
