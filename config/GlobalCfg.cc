#include "pdsapp/config/GlobalCfg.hh"

#include "pdsapp/config/Device.hh"
#include "pdsapp/config/Table.hh"
#include "pds/config/PdsDefs.hh"
#include "pds/config/DbClient.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace Pds_ConfigDb;

static const int bsize = 0x100000;
static char* _buffer = new char[bsize];
static char* _buffer_end = _buffer+bsize;
static char* _next;
static char* _btype[PdsDefs::NumberOf+1];

static void _clearAll()
{
  memset(_btype,0,sizeof(_btype));
  _next = _buffer;
}

static void _loadType(DbClient&          db,
                      const Pds::TypeId& id,
                      const std::string& name)
{
  //
  //  This is incomplete
  //
  unsigned i = PdsDefs::configType(id);
  if (i == PdsDefs::NumberOf) return;

  XtcEntry x;
  x.type_id = id;
  x.name    = name;

  db.begin();
  int sz = db.getXTC(x, _next, _buffer_end-_next);
  if (sz <= 0) {
    printf("GlobalCfg error reading %s\n",name.c_str());
    db.abort();
  }
  else {
    printf("GlobalCfg read %d bytes from %s\n",int(sz),name.c_str());
    _btype[i] = _next;
    _next += sz;
    db.commit();
  }
}

bool GlobalCfg::contains(const UTypeName& utype)
{
  const unsigned _types = 
    (1<<PdsDefs::EvrIO);

  PdsDefs::ConfigType t = PdsDefs::configType(*PdsDefs::typeId(utype));
  return (_types & (1<<t));
}

void GlobalCfg::cache(DbClient& path, const Device* device)
{
  _clearAll();
  
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

void* GlobalCfg::fetch(Pds::TypeId id)
{
  return _btype[PdsDefs::configType(id)];
}
