#ifndef Pds_PVControl_hh
#define Pds_PVControl_hh

#include "pds/config/ControlConfigType.hh"

#include <list>

namespace Pds {

  class ControlCA;
  class PVRunnable;

  class PVControl {
  public:
    PVControl (PVRunnable&);
    virtual ~PVControl();
  public:
    bool runnable() const;
  public:
    void configure  (const ControlConfigType&);
    void unconfigure();
    void channel_changed();
  private:
    PVRunnable&           _report;
    bool                  _runnable;
    std::list<ControlCA*> _channels;
  };
};

#endif
