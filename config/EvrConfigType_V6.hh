#ifndef Pds_EvrConfigType_V6_hh
#define Pds_EvrConfigType_V6_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/evr.ddl.h"

namespace Pds_ConfigDb {
  namespace EvrConfig_V6 {
    typedef Pds::EvrData::ConfigV6      EvrConfigType;
    typedef Pds::EvrData::EventCodeV5   EventCodeType;
    typedef Pds::EvrData::PulseConfigV3 PulseType;
    typedef Pds::EvrData::OutputMapV2   OutputMapType;
    typedef Pds::EvrData::SequencerConfigV1 SeqConfigType;

    static Pds::TypeId _evrConfigType(Pds::TypeId::Id_EvrConfig,
                                      EvrConfigType::Version);
    unsigned size(const EvrConfigType&);
  };
};

#endif
