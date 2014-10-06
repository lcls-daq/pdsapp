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
      for(unsigned i=0; i<Pds::TypeId::NumberOf+1; i++)
	insert((Pds::TypeId::Type)i,0,force);
    }
    void  insert(Pds::TypeId::Type q, char* p, bool force) 
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
    void  remove(Pds::TypeId::Type q, bool force)
    { insert(q,0,force); }
    char* fetch (Pds::TypeId::Type q) const { return q==Pds::TypeId::NumberOf ? 0:_btype[q]; }
  private:
    uint64_t _forced;
    char* _btype[Pds::TypeId::NumberOf+1];
  };
};

using namespace Pds_ConfigDb;

static GlobalCfg* _instance = 0;

void GlobalCfg::_loadType(DbClient&          db,
			  const Pds::TypeId& id,
			  const std::string& name)
{
  Pds::TypeId::Type i = id.id();

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
  const Pds::TypeId* id = PdsDefs::typeId(utype);
  if (!id) return false;

  return contains(*id);
}

bool GlobalCfg::contains(Pds::TypeId id)
{
  switch(id.id()) {
  case Pds::TypeId::Id_EvrIOConfig:
  case Pds::TypeId::Id_AliasConfig:
    return true;
  default:
    return false;
  }
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
  _cache.remove(id.id(),force);
}

void GlobalCfg::cache(const UTypeName& utype, char* p, bool force) 
{
  cache(*PdsDefs::typeId(utype),p,force);
}

void GlobalCfg::cache(Pds::TypeId id, char* p, bool force) 
{
  _cache.insert(id.id(),p,force); 

  std::list<Pds::Routine*>& l = _clients[id.value()];
  for(std::list<Pds::Routine*>::iterator it=l.begin(); it!=l.end(); it++)
    (*it)->routine();
}

void* GlobalCfg::fetch(Pds::TypeId id)
{
  return _cache.fetch(id.id());
}

void  GlobalCfg::enroll(Pds::Routine* r, Pds::TypeId id)
{
  _clients[id.value()].push_back(r);
}

void  GlobalCfg::resign(Pds::Routine* r)
{
  for(MapType::iterator it=_clients.begin(); it!=_clients.end(); it++)
    it->second.remove(r);
}

GlobalCfg& GlobalCfg::instance()
{
  if (!_instance) _instance = new GlobalCfg;
  return *_instance;
}

GlobalCfg::GlobalCfg() : _cache(*new GlobalCfgCache) {}

GlobalCfg::~GlobalCfg() { delete &_cache; }
