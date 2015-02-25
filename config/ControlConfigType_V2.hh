#ifndef Pds_ControlConfigType_V2_hh
#define Pds_ControlConfigType_V2_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/control.ddl.h"

#include <list>

typedef Pds::ControlData::ConfigV2 ControlConfigType;

static Pds::TypeId _controlConfigType(Pds::TypeId::Id_ControlConfig,
				      ControlConfigType::Version);

namespace Pds_ConfigDb {
  namespace ControlConfig_V2 {
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
  }
}

#endif
