#include "pdsapp/config/GlobalCfg.hh"

#include "pdsapp/config/Device.hh"
#include "pdsapp/config/Table.hh"
#include "pds/config/PdsDefs.hh"
#include "pds/config/DbClient.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace Pds_ConfigDb {
  class GlobalCfgCache {
  public:
    GlobalCfgCache() : _forced(0) { memset(_btype, 0, sizeof(_btype)); }
    ~GlobalCfgCache() { clear(true); }
  public:
    void  clear (bool force) { 
      for(unsigned i=0; i<PdsDefs::NumberOf+1; i++)
	insert((PdsDefs::ConfigType)i,0,force);
    }
    void  insert(PdsDefs::ConfigType q, char* p, bool force) 
    {
      if (force) {
	if (_btype[q]) delete[] _btype[q];
	_btype[q]=p; 
	if (p)
	  _forced |= (1<<q);
	else
	  _forced &= ~(1<<q);
      }
      else if (_forced & (1<<q)) {
	if (p) delete p;
      }
      else {
	if (_btype[q]) delete _btype[q];
	_btype[q] = p;
      }
    }
    void  remove(PdsDefs::ConfigType q, bool force)
    { insert(q,0,force); }
    char* fetch (PdsDefs::ConfigType q) const { return q==PdsDefs::NumberOf ? 0:_btype[q]; }
  private:
    uint64_t _forced;
    char* _btype[PdsDefs::NumberOf+1];
  };
};

using namespace Pds_ConfigDb;

GlobalCfgCache _cache;

static void _loadType(DbClient&          db,
                      const Pds::TypeId& id,
                      const std::string& name)
{
  PdsDefs::ConfigType i = PdsDefs::configType(id);

  //  Already cached?
  if (_cache.fetch(i)) return;

  XtcEntry x;
  x.type_id = id;
  x.name    = name;

  const unsigned _sz = 0x10000;
  char* _next = new char[_sz];

  db.begin();
  int sz = db.getXTC(x, _next, _sz);
  if (sz <= 0) {
    printf("GlobalCfg error reading %s\n",name.c_str());
    db.abort();
    delete[] _next;
  }
  else {
    printf("GlobalCfg read %d bytes from %s\n",int(sz),name.c_str());
    _cache.insert(i,_next,false);
    db.commit();
  }
}

bool GlobalCfg::contains(const UTypeName& utype)
{
  const unsigned _types = 
    (1<<PdsDefs::EvrIO);

  const Pds::TypeId* id = PdsDefs::typeId(utype);
  if (!id) return false;

  PdsDefs::ConfigType t = PdsDefs::configType(*id);
  return (_types & (1<<t));
}

void GlobalCfg::cache(DbClient& path, const Device* device)
{
  _cache.clear(false);

  // construct the list of types
  const TableEntry* entry = device->table().get_top_entry(name());
  if (entry) {
    for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	iter!=entry->entries().end(); iter++) {
      UTypeName stype(iter->name());
      _loadType(path,*PdsDefs::typeId(stype),iter->entry());
    }
  }
}

void GlobalCfg::flush(const UTypeName& utype,bool force)
{
  flush(*PdsDefs::typeId(utype),force);
}

void GlobalCfg::flush(Pds::TypeId id,bool force)
{
  _cache.remove(PdsDefs::configType(id),force);
}

void GlobalCfg::cache(const UTypeName& utype, char* p, bool force) 
{
  cache(*PdsDefs::typeId(utype),p,force);
}

void GlobalCfg::cache(Pds::TypeId id, char* p, bool force) 
{
  _cache.insert(PdsDefs::configType(id),p,force); 
}

void* GlobalCfg::fetch(Pds::TypeId id)
{
  return _cache.fetch(PdsDefs::configType(id));
}
