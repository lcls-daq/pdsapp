#ifndef Pds_ControlConfigType_V3_hh
#define Pds_ControlConfigType_V3_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/control.ddl.h"

#include <list>

typedef Pds::ControlData::ConfigV3 ControlConfigType;

namespace Pds_ConfigDb {
  namespace ControlConfig_V3 {
    class L3TEvents {
    public:
      L3TEvents(unsigned e) : _events(e) {}
      operator unsigned() const { return _events; }
    private:
      unsigned _events;
    };

    ControlConfigType* _new(void*);
    ControlConfigType* _new(void*, 
                            const std::list<Pds::ControlData::PVControl>&,
                            const std::list<Pds::ControlData::PVMonitor>&,
                            const std::list<Pds::ControlData::PVLabel>&,
                            const Pds::ClockTime&);
    ControlConfigType* _new(void*, 
                            const std::list<Pds::ControlData::PVControl>&,
                            const std::list<Pds::ControlData::PVMonitor>&,
                            const std::list<Pds::ControlData::PVLabel>&,
                            unsigned events);
    ControlConfigType* _new(void*, 
                            const std::list<Pds::ControlData::PVControl>&,
                            const std::list<Pds::ControlData::PVMonitor>&,
                            const std::list<Pds::ControlData::PVLabel>&,
                            L3TEvents events);
  }
}

#endif
