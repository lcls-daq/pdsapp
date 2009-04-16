#include "pdsapp/config/Device.hh"

#include "pdsapp/config/PdsDefs.hh"

#include <sys/stat.h>
#include <glob.h>

#include <sstream>
using std::istringstream;
using std::ostringstream;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <iomanip>
using std::setw;
using std::setfill;

using namespace Pds_ConfigDb;

DeviceEntry::DeviceEntry(unsigned id) :
  Pds::Src(Pds::Level::Source)
{
  _phy = id; 
}

DeviceEntry::DeviceEntry(const string& id) 
{
  char sep;
  istringstream i(id);
  i >> std::hex >> _log >> sep >> _phy;
}

string DeviceEntry::id() const
{
  ostringstream o;
  o << std::hex << setfill('0') << setw(8) << _log << '.' << setw(8) << _phy;
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
  ostringstream o;
  o << path << "/keys/" << _name << "/" << key;
  return o.str();
}

string Device::typepath(const string& path, 
			const string& key, 
			const UTypeName& entry)
{
  const Pds::TypeId* typeId = PdsDefs::typeId(entry);
  ostringstream o;
  o << path << "/keys/" << _name << "/" << std::hex << setfill('0') << setw(8) << key << "/" << setw(8) << typeId->value();
  return o.str();
}

string Device::typelink(const UTypeName& uname, const string& entry)
{
  ostringstream o;
  o << "../../../xtc/" << PdsDefs::qtypeName(uname) << "/" << entry;
  return o.str();
}

// Checks that the key components are valid
bool Device::validate_key(const string& config, const string& path)
{    
  bool invalid  = false;
  struct stat s;
  const TableEntry* entry = _table.get_top_entry(config);
  if (!entry) return false;
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
    UTypeName utype(iter->name());
    string tlink = typelink(utype,iter->entry());
    if (!stat(tlink.c_str(),&s)) {
      cerr << "Found archaic type entry " << _name << "/" << utype << "/" << iter->entry() << endl
	   << "The " << _name << " version may have changed." << endl;
      invalid = true;
    }
  }
  return !invalid;
}

// Checks that the key link exists and is up to date
bool Device::update_key(const string& config, const string& path)
{    
  const int line_size=128;
  char buff[line_size];
  bool outofdate=false;
  const TableEntry* entry = _table.get_top_entry(config);
  if (!entry) return false;
  string kpath = keypath(path,entry->key());
  struct stat s;
  if (stat(kpath.c_str(),&s)) outofdate=true;
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
    UTypeName utype(iter->name());
    string tpath = typepath(path,entry->key(),utype);
    string tlink = typelink(utype,iter->entry());
    if (!stat(tpath.c_str(),&s)) {
      int sz=readlink(tpath.c_str(),buff,line_size);
      if (sz<0) outofdate=true;
      else {
	buff[sz] = 0;
	if (strcmp(buff,tlink.c_str())) 
	  outofdate=true;
      }
    }
    else {
      outofdate=true;
    }
  }

  if (outofdate) {
    glob_t g;
    glob(keypath(path,"[0-9]*").c_str(),0,0,&g);
    sprintf(buff,"%08x",g.gl_pathc);
    globfree(&g);
    string key = string(buff);
    string kpath = keypath(path,key);
    mode_t mode = S_IRWXU | S_IRWXG;
    mkdir(kpath.c_str(),mode);
    for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
      UTypeName utype(iter->name());
      string tpath = typepath(path,key,utype);
      string tlink = typelink(utype,iter->entry());
      symlink(tlink.c_str(), tpath.c_str());
    }
    TableEntry t(entry->name(), key, entry->entries());
    _table.set_top_entry(t);
  }
  return outofdate;
}
