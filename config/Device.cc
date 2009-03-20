#include "pdsapp/config/Device.hh"

#include "pdsapp/config/PdsDefs.hh"

#include <sys/stat.h>
#include <glob.h>

#include <sstream>
using std::istringstream;
using std::ostringstream;

using namespace Pds_ConfigDb;

DeviceEntry::DeviceEntry(unsigned id) :
  Pds::Src(Pds::Level::Source)
{
  _phy = id; 
}

DeviceEntry::DeviceEntry(const string& id) :
  Pds::Src(Pds::Level::Source)
{
  istringstream i(id);
  i >> std::hex >> _phy;
}

string DeviceEntry::id() const
{
  ostringstream o;
  o.width(8);
  o << std::hex << _phy;
  return o.str();
}

const Pds::DetInfo& DeviceEntry::info() const
{
  return reinterpret_cast<const Pds::DetInfo&>(*this);
}


Device::Device(const string& path,
	       const string& name,
	       const list<DeviceEntry>& src_list) :
  _name (name),
  _table(path),
  _src_list(src_list)
{
}

string Device::keypath(const string& path, const string& key)
{
  return path + "/keys/" + _name + "/" + key;
}

string Device::typepath(const string& path, const string& key, const string& entry)
{
  char buff[16];
  sprintf(buff,"%08x",PdsDefs::type_index(entry));
  return path + "/keys" + _name + "/" + key + "/" + string(buff);
}

string Device::typelink(const string& name, const string& entry)
{
  return string("../../../xtc/") + name + "/" + entry;
}

// Checks that the key link exists and is up to date
bool Device::validate_key(const string& config, const string& path)
{    
  const int line_size=128;
  char buff[line_size];
  bool invalid=false;
  const TableEntry* entry = _table.get_top_entry(config);
  if (!entry) return false;
  string kpath = keypath(path,entry->key());
  struct stat s;
  if (stat(kpath.c_str(),&s)) invalid=true;
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
    string tpath = typepath(path,entry->key(),iter->name());
    if (!stat(tpath.c_str(),&s)) {
      unsigned sz=line_size;
      if (!(readlink(tpath.c_str(),buff,sz) && 
	    string(buff)==typelink(iter->name(),iter->entry())))
	invalid=true;
    }
  }

  if (invalid) {
    glob_t g;
    glob(keypath(path,"[0-9]*").c_str(),0,0,&g);
    sprintf(buff,"%08x",g.gl_pathc);
    globfree(&g);
    string key = string(buff);
    string kpath = keypath(path,key);
    mode_t mode = S_IRWXU | S_IRWXG;
    mkdir(kpath.c_str(),mode);
    for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++)
      symlink(typelink(iter->name(),iter->entry()).c_str(),
	      typepath(path,key,iter->name()).c_str());
    TableEntry t(entry->name(), key, entry->entries());
    _table.set_top_entry(*entry);
  }
  return invalid;
}
