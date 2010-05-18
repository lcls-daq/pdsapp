#ifndef PdsConfigDb_GlobalCfg_hh
#define PdsConfigDb_GlobalCfg_hh

#include "pdsdata/xtc/TypeId.hh"

namespace Pds_ConfigDb {
  class Device;
  class Path;

  class GlobalCfg {
  public:
    static const char* name();
    static void  cache(const Path&,Device*);
    static void* fetch(Pds::TypeId);    // if data for that type is cached
    static bool  contains(Pds::TypeId); // if the type is global
  };
};

#endif
