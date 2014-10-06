#ifndef PdsConfigDb_GlobalCfg_hh
#define PdsConfigDb_GlobalCfg_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pds/config/PdsDefs.hh"
#include "pds/service/Routine.hh"

#include <list>
#include <map>

namespace Pds_ConfigDb {
  class Device;
  class DbClient;
  class GlobalCfgCache;

  class GlobalCfg {
  public:
    const char* name() { return PdsDefs::GlobalCfg_name(); }
    void  cache(DbClient&,const Device*);
    void  cache(const UTypeName&,char*,bool force=false);
    void  cache(Pds::TypeId,char*,bool force=false);
    void  flush(const UTypeName&,bool force=false);
    void  flush(Pds::TypeId,bool force=false);
    void* fetch(Pds::TypeId);    // if data for that type is cached
    bool  contains(const UTypeName&); // if the type is global
    bool  contains(Pds::TypeId);      // if the type is global
  public:
    void  enroll(Pds::Routine*, Pds::TypeId);
    void  resign(Pds::Routine*);
  public:
    static GlobalCfg& instance();
  private:
    GlobalCfg();
    ~GlobalCfg();
  private:
    void _loadType(DbClient&, const Pds::TypeId&, const std::string&);
  private:
    GlobalCfgCache& _cache;
    typedef std::map< unsigned, std::list<Pds::Routine*> > MapType;
    MapType _clients;
  };
};

#endif
