#ifndef Pds_EvrConfigType_V6_hh
#define Pds_EvrConfigType_V6_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/evr/ConfigV6.hh"
#include "pdsdata/evr/DataV3.hh"

namespace Pds_ConfigDb {
  namespace EvrConfig_V6 {
    typedef Pds::EvrData::ConfigV6  EvrConfigType;
    typedef Pds::EvrData::DataV3    EvrDataType;

    static Pds::TypeId _evrConfigType(Pds::TypeId::Id_EvrConfig,
                                      EvrConfigType::Version);
  };
};

#endif
