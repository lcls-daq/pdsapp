#ifndef Pds_EvrConfigType_hh
#define Pds_EvrConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/evr.ddl.h"

typedef Pds::EvrData::ConfigV5  EvrConfigType;
typedef Pds::EvrData::DataV3    EvrDataType;

static Pds::TypeId _evrConfigType(Pds::TypeId::Id_EvrConfig,
          EvrConfigType::Version);

#endif
