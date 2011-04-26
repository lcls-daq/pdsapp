#include "pdsapp/config/GlobalCfg.hh"

#include "pdsapp/config/Device.hh"
#include "pdsapp/config/Path.hh"
#include "pdsapp/config/PdsDefs.hh"

#include <stdlib.h>
#include <stdio.h>

using namespace Pds_ConfigDb;

static const char* globalAlias = "_GLOBAL_";

static char* _buffer = new char[0x100000];
static char* _next;
static char* _btype[PdsDefs::NumberOf+1];

static void _clearAll()
{
  memset(_btype,0,sizeof(_btype));
  _next = _buffer;
}

static void _loadType(const Pds::TypeId& id,
                      const char* file)
{
  //
  //  This is incomplete
  //
  unsigned i = PdsDefs::configType(id);
  if (i == PdsDefs::NumberOf) return;

  FILE* f = fopen(file,"r");
  if (f) {
    size_t sz = fread(_next, 1, 0x7ffffff, f);
    if (sz < 0) 
      printf("GlobalCfg error reading %s\n",file);
    else {
      printf("GlobalCfg read %d bytes from %s\n",int(sz),file);
      _btype[i] = _next;
      _next += sz;
    }
    fclose(f);
  }
}

const char* GlobalCfg::name() { return globalAlias; }

bool GlobalCfg::contains(const UTypeName& utype)
{
  const unsigned _types = 
    (1<<PdsDefs::EvrIO);

  PdsDefs::ConfigType t = PdsDefs::configType(*PdsDefs::typeId(utype));
  return (_types & (1<<t));
}

void GlobalCfg::cache(const Path& path, Device* device)
{
  _clearAll();
  
  // construct the list of types
  const TableEntry* entry = device->table().get_top_entry(globalAlias);
  if (entry) {
    for(list<FileEntry>::const_iterator iter=entry->entries().begin();
	iter!=entry->entries().end(); iter++) {
      UTypeName stype(iter->name());
      string spath(path.data_path("",stype));
      string qfile = spath + "/" + iter->entry();
      _loadType(*PdsDefs::typeId(stype),qfile.c_str());
    }
  }
}

void* GlobalCfg::fetch(Pds::TypeId id)
{
  return _btype[PdsDefs::configType(id)];
}
