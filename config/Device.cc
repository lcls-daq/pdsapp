#include "pdsapp/config/Device.hh"

#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/GlobalCfg.hh"

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


Device::Device(const string& path,
	       const string& name,
	       const list<DeviceEntry>& src_list) :
  _name (name),
  _table(path),
  _src_list(src_list)
{
}

bool Device::operator==(const Device& d) const
{
  return name() == d.name(); 
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

string Device::xtcpath(const string& path, const UTypeName& uname, const string& entry) const
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

  entry = _table.get_top_entry(string(GlobalCfg::name()));
  if (entry) {
    for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
      UTypeName utype(iter->name());
      string tlink = typelink(utype,iter->entry());
      if (!stat(tlink.c_str(),&s)) {
	cerr << "Found archaic type entry " << _name << "/" << utype << "/" << iter->entry() << endl
	     << "The " << _name << " version may have changed." << endl;
	invalid = true;
      }
    }
  }

  return !invalid;
}

bool Device::_check_config(const TableEntry* entry, const string& path, const string& key)
{
  const int line_size=128;
  char buff[line_size];
  bool outofdate = false;
  string kpath = keypath(path,key);
  struct stat s;
  if (stat(kpath.c_str(),&s)) { outofdate=true; }
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
    UTypeName utype(iter->name());
    string tpath = typepath(path,key,utype);
    string tlink = typelink(utype,iter->entry());
    if (!stat(tpath.c_str(),&s)) {
      int sz=readlink(tpath.c_str(),buff,line_size);
      if (!iter->enabled()) { outofdate=true; }
      else if (sz<0) { outofdate=true; }
      else {
	//
	//  Test that the symbolic link points to file equivalent to the xtc file
	//
	string tlinkpath = tpath.substr(0,tpath.find_last_of("/"))+"/"+tlink;
	struct stat ls;
	if (!stat(tlinkpath.c_str(),&ls) && s.st_size==ls.st_size) {
	  FILE* f_link = fopen(tlinkpath.c_str(),"r");
	  if (!f_link) {
	    perror(tlinkpath.c_str());
	    printf("Failed to open link, recreating.\n");
	    outofdate=true;
	    continue;
	  }
	  FILE* f_path = fopen(tpath.c_str(),"r");
	  if (!f_path) {
	    perror(tpath.c_str());
	    printf("Failed to open file, recreating.\n");
	    outofdate=true;
	    fclose(f_link);
	    continue;
	  }
	  char* bufflink = new char[s.st_size];
	  char* buffpath = new char[s.st_size];
	  bool equal_size = 
	    fread(bufflink, s.st_size, 1, f_link)==
	    fread(buffpath, s.st_size, 1, f_path);
	  fclose(f_path);
	  fclose(f_link);
	  bool equal_mem = memcmp(bufflink,buffpath,s.st_size)==0;
	  delete[] bufflink;
	  delete[] buffpath;
	  if (!equal_size || !equal_mem) {
	    printf("%s link is old\n",tlinkpath.c_str());
	    outofdate=true; 
	  }
	}
	else {
          //	  printf("No %s link\n",tlinkpath.c_str());
	  outofdate=true;
	}
      }
    }
    else if (iter->enabled()) {
      //      printf("No %s path\n",tpath.c_str());
      outofdate=true;
    }
    else   // disabled component should be absent
      ;
  }
  return outofdate;
}

void Device::_make_config(const TableEntry* entry, const string& path, const string& key)
{
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
    if (!iter->enabled())
      continue;

    UTypeName utype(iter->name());
    string tpath = typepath(path,key,utype);
    //
    //  If the latest versioned xtc file is up-to-date, point to it; else make another version.
    //
    string tbase = xtcpath(path,utype,iter->entry());

    int nv=0;
    glob_t gv;
    string tvsns = tbase + ".[0-9]*";
    glob(tvsns.c_str(),0,0,&gv);
    nv = gv.gl_pathc;               // count of numeric extensions
    globfree(&gv);

    if (nv>0) {
      string sext;                  // latest numeric extension
      { ostringstream o;
	o  << "." << nv-1;
	sext = o.str(); }
      string talnk = tbase + sext;  // path from cwd
      struct stat s, ls;
      stat(tpath.c_str(),&s);
      stat(talnk.c_str(),&ls);
      if (s.st_size==ls.st_size) {
	FILE* f_base = fopen(tbase.c_str(),"r");
	FILE* f_link = fopen(talnk.c_str(),"r");
	if (!f_base || !f_link) {
	  fprintf(stderr,"Error opening files {");
	  if (!f_base) fprintf(stderr," %s",tbase.c_str());
	  if (!f_link) fprintf(stderr," %s",talnk.c_str());
	  fprintf(stderr," } to add config link\n");
	  abort();
	}
	char* buffbase = new char[s.st_size];
	char* bufflink = new char[s.st_size];
	bool equal_size = 
	  fread(buffbase, s.st_size, 1, f_base)==
	  fread(bufflink, s.st_size, 1, f_link);
	fclose(f_base);
	fclose(f_link);
	bool equal_mem = memcmp(buffbase,bufflink,s.st_size)==0;
	delete[] buffbase;
	delete[] bufflink;
	if (equal_size && equal_mem) {
	  printf("%s is up-to-date\n",talnk.c_str());
	  string tlink = typelink(utype,iter->entry())+sext; // path from key
	  symlink(tlink.c_str(), tpath.c_str());
	  continue;
	}
      }
    }
    //
    //  make a new version and link to it
    //
    { ostringstream o;
      o << "cp " << tbase << " " << tbase << "." << nv;
      system(o.str().c_str()); }
    { ostringstream o;
      o << typelink(utype,iter->entry()) << "." << nv;
      symlink(o.str().c_str(), tpath.c_str()); }
  }
}

// Checks that the key link exists and is up to date
bool Device::update_key(const string& config, const string& path)
{    
  const int line_size=128;
  char buff[line_size];
  bool outofdate=false;
  const TableEntry* entry = _table.get_top_entry(config);
  if (!entry) return false;

  outofdate |= _check_config(entry, path, entry->key());

  const TableEntry* gentry = _table.get_top_entry(string(GlobalCfg::name()));
  if (gentry)
    outofdate |= _check_config(gentry, path, entry->key());

  if (outofdate) {
    glob_t g;
    glob(keypath(path,"[0-9]*").c_str(),0,0,&g);
    sprintf(buff,"%08x",unsigned(g.gl_pathc));
    globfree(&g);
    string key = string(buff);
    string kpath = keypath(path,key);
    //    mode_t mode = S_IRWXU | S_IRWXG;
    mode_t mode = _fmode;
    mkdir(kpath.c_str(),mode);

    _make_config(entry, path, key);

    if (gentry)
      _make_config(gentry, path, key);

    TableEntry t(entry->name(), key, entry->entries());
    _table.set_top_entry(t);
  }
  return outofdate;
}
