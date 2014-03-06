#include "pdsapp/config/Experiment.hh"
#include "pds/config/PdsDefs.hh"
#include "pds/config/DbClient.hh"

#include <stdio.h>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <sstream>
using std::ostringstream;
#include <iomanip>
using std::setw;
using std::setfill;
using std::hex;

#include <string>
#include <string.h>
#include <stdlib.h>

#define DBUG

static void _handle_no_lock(const char* s)
{
  std::string _no_lock_exception(s);
  printf("_handle_no_lock:%s\n",s);
  throw _no_lock_exception;
}

#ifdef DBUG
static inline double time_diff(struct timespec& b, 
                               struct timespec& e)
{
  return double(e.tv_sec - b.tv_sec) + 1.e-9*
    (double(e.tv_nsec) - double(b.tv_nsec));
}
#endif

using namespace Pds_ConfigDb;

Experiment::Experiment(const char* path, Option lock) :
  _lock(lock)
{
  if (!(_db = DbClient::open(path))) {
    perror("DbClient::open");
    abort();
  }

  if (_lock==Lock)
    _db->begin();
}

Experiment::~Experiment()
{
  if (_lock==Lock)
    _db->abort();
}

void Experiment::load()
{
  //
  //  Load the experiment alias table from the database
  //
  _table.entries().clear();

  std::list<ExptAlias> aliases = _db->getExptAliases();
  for(std::list<ExptAlias>::iterator it=aliases.begin();
      it!=aliases.end(); it++) {
    ostringstream key;
    key << std::hex << setw(8) << setfill('0') << it->key;

    std::list<FileEntry> entries;
    for(std::list<DeviceEntryRef>::iterator e_it=it->devices.begin();
        e_it!=it->devices.end(); e_it++)
      entries.push_back( FileEntry( e_it->device, e_it->name ) );

    _table.entries().push_back( TableEntry(it->name, key.str(), entries) );
  }

  //
  //  Load the devices alias table from the database
  //
  _devices.clear();

  std::list<DeviceType> devices = _db->getDevices();
  for(std::list<DeviceType>::iterator it=devices.begin();
      it!=devices.end(); it++) {
    
    Table table;
    for(std::list<DeviceEntries>::iterator e_it=it->entries.begin();
        e_it!=it->entries.end(); e_it++) {
      
      std::list<FileEntry> entries;
      for(std::list<XtcEntry>::iterator x_it=e_it->entries.begin();
          x_it!=e_it->entries.end(); x_it++)
        entries.push_back( FileEntry( Pds::TypeId::name(x_it->type_id.id()), x_it->name ) );
        
      table.entries().push_back( TableEntry(e_it->name, "ffffffff", entries) );
    }

    std::list<DeviceEntry> sources;
    for(std::list<uint64_t>::iterator s_it=it->sources.begin();
        s_it!=it->sources.end(); s_it++)
      sources.push_back( DeviceEntry(*s_it) );

    _devices.push_back( Device(it->name, table, sources) );
  }
}

void Experiment::save() const
{
  std::list<DeviceType> dlist;

  for(std::list<Device>::const_iterator it=_devices.begin();
      it!=_devices.end(); it++) {
    DeviceType d;
    d.name    = it->name();

    for(std::list<TableEntry>::const_iterator t_it=it->table().entries().begin();
        t_it!=it->table().entries().end(); t_it++) {
      DeviceEntries entry;
      entry.name = t_it->name();

      for(std::list<FileEntry>::const_iterator f_it=t_it->entries().begin();
          f_it!=t_it->entries().end(); f_it++) {
        XtcEntry x;
        x.type_id = *PdsDefs::typeId(UTypeName(f_it->name()));
        x.name    = f_it->entry();
        entry.entries.push_back(x);
      }
      d.entries.push_back(entry);
    }

    for(std::list<DeviceEntry>::const_iterator d_it=it->src_list().begin();
        d_it!=it->src_list().end(); d_it++) {
      uint64_t lp = d_it->log();
      lp = (lp<<32) | d_it->phy();
      d.sources.push_back(lp);
    }

    dlist.push_back(d);
  }

  _db->begin();
  if (_db->setDevices(dlist)!=0) {
    printf("DbClient::setDevices failed\n");
    _db->abort();
  }
  else
    _db->commit();

  std::list<ExptAlias> elist;

  for(std::list<TableEntry>::const_iterator it=_table.entries().begin();
      it!=_table.entries().end(); it++) {
    ExptAlias entry;
    entry.name = it->name();
    entry.key  = strtoul(it->key().c_str(),NULL,16);

    for(std::list<FileEntry>::const_iterator f_it=it->entries().begin();
        f_it!=it->entries().end(); f_it++) {
      DeviceEntryRef x;
      x.device  = f_it->name();
      x.name    = f_it->entry();
      entry.devices.push_back(x);
    }

    elist.push_back(entry);
  }

  _db->begin();
  if (_db->setExptAliases(elist)!=0) {
    printf("DbClient::setExptAlias failed\n");
    _db->abort();
  }
  else
    _db->commit();
}

void Experiment::read() 
{
  load();
}

void Experiment::write() const
{
  save();
}

Device* Experiment::device(const string& name)
{
  if (_lock==NoLock)
    _handle_no_lock("device");

  for(list<Device>::iterator iter=_devices.begin(); iter!=_devices.end(); iter++)
    if (iter->name() == name)
      return &(*iter);
  return 0;
}

const Device* Experiment::device(const string& name) const
{
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++)
    if (iter->name() == name)
      return &(*iter);
  return 0;
}

void Experiment::add_device(const string& name,
                            const list<DeviceEntry>& slist)
{
  if (_lock==NoLock) 
    _handle_no_lock("add_device");

  Device device(name,Table(),slist);
  _devices.push_back(device);
}

void Experiment::remove_device(const Device& device)
{
  if (_lock==NoLock) 
    _handle_no_lock("remove_device");

  for(list<TableEntry>::iterator alias=_table.entries().begin();
      alias!=_table.entries().end(); alias++)
    for(list<FileEntry>::const_iterator iter=alias->entries().begin();
        iter != alias->entries().end(); iter++) {
      FileEntry e(*iter);
      if (e.name() == device.name()) {
        alias->remove(e);
        break;
      }
    }

  _devices.remove(device);
}

void Experiment::import_data(const string& device,
                             const UTypeName& type,
                             const string& file,
                             const string& desc)
{
  if (_lock==NoLock) 
    _handle_no_lock("import_data");

  if (PdsDefs::typeId(type)==0) {
    cerr << type << " not registered as valid configuration data type" << endl;
    return;
  }
}

bool Experiment::update_key_file(const TableEntry& entry)
{
  if (_lock==NoLock) 
    _handle_no_lock("update_key_file");

  return true;
}

unsigned Experiment::next_key_file() const
{
  return next_key();
}

unsigned Experiment::next_key() const { return _table._next_key; }

void Experiment::update_keys()
{
#ifdef DBUG
  struct timespec tv_b;
  clock_gettime(CLOCK_REALTIME,&tv_b);
#endif

  if (_lock==NoLock) 
    _handle_no_lock("update_keys");

  save();

  _db->begin();
  _db->updateKeys();
  _db->commit();

  load();

#ifdef DBUG
  struct timespec tv_e;
  clock_gettime(CLOCK_REALTIME,&tv_e);
  printf("Experiment::update_keys() %f s\n", time_diff(tv_b,tv_e));
#endif
}

int Experiment::current_key(const string& alias) const
{
  const TableEntry* e = _table.get_top_entry(alias.c_str());
  return e ? strtoul(e->key().c_str(),NULL,16) : -1;
}

//
//  Clone an existing key
//
unsigned Experiment::clone(const string& alias)
{
  if (_lock==NoLock)
    _handle_no_lock("clone");

  TableEntry* iter = const_cast<TableEntry*>(_table.get_top_entry(alias));
  if (!iter)
    throw std::string("alias not found");

  unsigned okey = strtoul(iter->key().c_str(),NULL,16);

  _db->begin();
  std::list<KeyEntry> entries = _db->getKey(okey);
  Key key;
  key.key  = _db->getNextKey();
  key.time = 0;
  key.name = alias;
  if (_db->setKey(key,entries)!=0) {
    printf("Error cloning key %d (->%d)\n",okey,key.key);
    _db->abort();
  }
  else
    _db->commit();

  return key.key;
}

void Experiment::substitute(unsigned           key, 
                            const string&      devname,
                            const Pds::TypeId& type, 
                            const char*        payload,
                            size_t             sz) const
{
  if (_lock==NoLock) 
    _handle_no_lock("substitute");

  const Device* dev = device(devname);
  if (!dev) return;

  ostringstream skey;
  skey << std::hex << setw(8) << setfill('0') << key
       << devname;
  XtcEntry x;
  x.type_id = type;
  x.name    = skey.str();
  _db->begin();
  _db->setXTC( x, payload, sz );
  _db->commit();

  for(std::list<DeviceEntry>::const_iterator it=dev->src_list().begin(); it!=dev->src_list().end(); it++) {
    _substitute(key, *it, type, x.name.c_str());
  }
}

void Experiment::substitute(unsigned           key, 
                            const Pds::Src&    src,
                            const Pds::TypeId& type, 
                            const char*        payload,
                            size_t             sz) const
{
  if (_lock==NoLock)
    _handle_no_lock("substitute_");

  ostringstream skey;
  skey << std::hex << setw(8) << setfill('0') << key
       << std::hex << setw(8) << setfill('0') << src.phy();
  XtcEntry x;
  x.type_id = type;
  x.name    = skey.str();
  _db->begin();
  _db->setXTC( x, payload, sz );
  _db->commit();

  _substitute(key,src,type,x.name.c_str());
}

void Experiment::_substitute(unsigned           key, 
                             const Pds::Src&    src,
                             const Pds::TypeId& type, 
                             const char*        name) const
{
  _db->begin();
  std::list<KeyEntry> entries = _db->getKey(key);
  _db->commit();

  DeviceEntry _src(src);
  for(std::list<KeyEntry>::iterator it=entries.begin();
      it!=entries.end(); it++) {
    if (DeviceEntry(it->source) == _src &&
        it->xtc.type_id.value() == type.value())
      it->xtc.name = std::string(name);
  }

  Key k;
  k.key =key;
  k.time=0;
  _db->begin();
  _db->setKey(k,entries);
  _db->commit();
}

void Experiment::dump() const
{
  std::string no_path;
  cout << "Experiment " << endl;
  _table.dump(no_path);
  //  cout << endl << _path.devices() << endl;
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    cout << iter->name();
    for(list<DeviceEntry>::const_iterator diter=iter->src_list().begin();
        diter!=iter->src_list().end(); diter++)
      cout << '\t' << std::hex << setfill('0') 
           << setw(8) << diter->log() << "."
           << setw(8) << diter->phy();
    cout << endl;
  }
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    iter->table().dump(no_path);
  }
}

void Experiment::log_threshold(double v) {}
