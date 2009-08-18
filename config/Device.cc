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

const mode_t _fmode = S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP | S_IRWXU;

DeviceEntry::DeviceEntry(unsigned id) :
  Pds::Src(Pds::Level::Source)
{
  _phy = id; 
}

DeviceEntry::DeviceEntry(const Pds::Src& id) :
  Pds::Src(id)
{
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

string Device::xtcpath(const string& path, const UTypeName& uname, const string& entry)
{
  ostringstream o;
  o << path << "/xtc/" << PdsDefs::qtypeName(uname) << "/" << entry;
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
#if 0
	//
	//  Test that the symbolic link points to the xtc file of the same name
	//
	buff[sz] = 0;
	if (strcmp(buff,tlink.c_str())) 
	  outofdate=true;
#else
	//
	//  Test that the symbolic link points to an equivalent file as the xtc file
	//
	string tlinkpath = tpath+"/"+tlink;
	struct stat ls;
	if (!stat(tlinkpath.c_str(),&ls) && s.st_size==ls.st_size) {
	  FILE* f_link = fopen(tlinkpath.c_str(),"r");
	  FILE* f_path = fopen(tpath.c_str(),"r");
	  char* bufflink = new char[s.st_size];
	  char* buffpath = new char[s.st_size];
	  if (fread(bufflink, s.st_size, 1, f_link)!=fread(buffpath, s.st_size, 1, f_path) ||
	      memcmp(bufflink,buffpath,s.st_size)!=0) {
	    printf("%s link is old\n",tlinkpath.c_str());
	    outofdate=true;
	  }
	}
	else {
	  printf("No %s link\n",tlinkpath.c_str());
	  outofdate=true;
	}
#endif
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
    //    mode_t mode = S_IRWXU | S_IRWXG;
    mode_t mode = _fmode;
    mkdir(kpath.c_str(),mode);
    for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
      UTypeName utype(iter->name());
      string tpath = typepath(path,key,utype);
#if 0
      string tlink = typelink(utype,iter->entry());
      symlink(tlink.c_str(), tpath.c_str());
#else
      //
      //  If the latest versioned xtc file is up-to-date, point to it; else make another version.
      //
      string tbase = xtcpath(path,utype,iter->entry());

      int nv=0;
      glob_t gv;
      string tvsns = tbase + ".[0-9]*";
      glob(tvsns.c_str(),0,0,&gv);
      nv = gv.gl_pathc;
      globfree(&gv);

      if (nv>0) {
	ostringstream o;
	o << tbase << "." << nv-1;
	string tlink = o.str();
	struct stat ls;
	stat(tpath.c_str(),&s);
	stat(tlink.c_str(),&ls);
	if (s.st_size==ls.st_size) {
	  FILE* f_base = fopen(tbase.c_str(),"r");
	  FILE* f_link = fopen(tlink.c_str(),"r");
	  char* buffbase = new char[s.st_size];
	  char* bufflink = new char[s.st_size];
	  if (fread(buffbase, s.st_size, 1, f_base)==fread(bufflink, s.st_size, 1, f_link) &&
	      memcmp(buffbase,bufflink,s.st_size)==0) {
	    printf("%s is up-to-date\n",tlink.c_str());
	    symlink(tlink.c_str(), tpath.c_str());
	    continue;
	  }
	}
      }
      { ostringstream o;
	o << "cp " << tbase << " " << tbase << "." << nv;
	system(o.str().c_str()); }
      { ostringstream o;
	o << tbase << "." << nv;
	symlink(o.str().c_str(), tpath.c_str()); }
#endif
    }
    TableEntry t(entry->name(), key, entry->entries());
    _table.set_top_entry(t);
  }
  return outofdate;
}
