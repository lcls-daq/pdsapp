#ifndef Pds_EvrConfigType_V3_hh
#define Pds_EvrConfigType_V3_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/evr.ddl.h"

namespace Pds_ConfigDb {
  namespace EvrConfig_V3 {
    typedef Pds::EvrData::ConfigV3      EvrConfigType;
    typedef Pds::EvrData::EventCodeV3   EventCodeType;
    typedef Pds::EvrData::PulseConfigV3 PulseType;
    typedef Pds::EvrData::OutputMap     OutputMapType;

    static Pds::TypeId _evrConfigType(Pds::TypeId::Id_EvrConfig,
                                      EvrConfigType::Version);
  };
};

#endif
