#include "pdsapp/config/Device.hh"

#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/Path.hh"
#include "pdsapp/config/XtcTable.hh"
#include "pdsapp/config/XML.hh"

#include <sys/stat.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define DBUG

static int _symlink(const char* dst, const char* src) {
  int r = symlink(dst,src);
  if (r<0) {
    char buff[256];
    sprintf(buff,"symlink %s -> %s",src,dst);
    perror(buff);
  }
  return r;
}

using namespace Pds_ConfigDb;

const mode_t _fmode = S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP | S_IRWXU;

Device::Device() :
  _name("None")
{
}

Device::Device(const string& path,
	       const string& name,
	       const list<DeviceEntry>& src_list) :
  _name (name),
  _table(path),
  _src_list(src_list)
{
  glob_t g;
  glob(keypath(path,"[0-9]*").c_str(),0,0,&g);
  _table._next_key = g.gl_pathc;
  globfree(&g);
}

void Device::load(const char*& p)
{
  _src_list.clear();

  XML_iterate_open(p,tag)
    if      (tag.name == "_name")
      _name = XML::IO::extract_s(p);
    else if (tag.name == "_table") {
      _table = Table();
      _table.load(p);
    }
    else if (tag.name == "_src_list") {
      DeviceEntry e;
      e.load(p);
      _src_list.push_back(e);
    }
  XML_iterate_close(Device,tag);
}

void Device::save(char*& p) const
{
  XML_insert(p, "string", "_name" , XML::IO::insert(p,_name));
  XML_insert(p, "Table" , "_table", _table.save(p));
  for(list<DeviceEntry>::const_iterator it=_src_list.begin(); it!=_src_list.end(); it++) {
    XML_insert(p, "DeviceEntry", "_src_list", it->save(p) );
  }
}

bool Device::operator==(const Device& d) const
{
  return name() == d.name(); 
}

string Device::keypath(const string& path, const string& key) const
{
  ostringstream o;
  o << path << "/keys/" << _name << "/" << key;
  return o.str();
}

string Device::typepath(const string& path, 
			const string& key, 
			const UTypeName& entry) const
{
  const Pds::TypeId* typeId = PdsDefs::typeId(entry);
  ostringstream o;
  o << path << "/keys/" << _name << "/" << std::hex << setfill('0') << setw(8) << key << "/" << setw(8) << typeId->value();
  return o.str();
}

string Device::typelink(const UTypeName& uname, const string& entry) const
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

bool Device::validate_key_file(const string& config, const string& path)
{    
  bool invalid  = false;
  struct stat64 s;
  const TableEntry* entry = _table.get_top_entry(config);
  if (!entry) return false;
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
    UTypeName utype(iter->name());
    string tlink = typelink(utype,iter->entry());
    if (!stat64(tlink.c_str(),&s)) {
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
      if (!stat64(tlink.c_str(),&s)) {
	cerr << "Found archaic type entry " << _name << "/" << utype << "/" << iter->entry() << endl
	     << "The " << _name << " version may have changed." << endl;
	invalid = true;
      }
    }
  }

  return !invalid;
}

bool Device::_check_config_file(const TableEntry* entry, const string& path, const string& key)
{
  const int line_size=128;
  char buff[line_size];
  bool outofdate = false;
  string kpath = keypath(path,key);
  struct stat64 s;
  if (stat64(kpath.c_str(),&s)) { outofdate=true; }
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {
    UTypeName utype(iter->name());
    string tpath = typepath(path,key,utype);
    string tlink = typelink(utype,iter->entry());
    if (!stat64(tpath.c_str(),&s)) {
      int sz=readlink(tpath.c_str(),buff,line_size);
      if (sz<0) { outofdate=true; }
      else {
	//
	//  Test that the symbolic link points to file equivalent to the xtc file
	//
	string tlinkpath = tpath.substr(0,tpath.find_last_of("/"))+"/"+tlink;
	struct stat64 ls;
	if (!stat64(tlinkpath.c_str(),&ls) && s.st_size==ls.st_size) {
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
    else
      outofdate=true;
  }
  return outofdate;
}

void Device::_make_config_file(const TableEntry* entry, const string& path, const string& key)
{
  for(list<FileEntry>::const_iterator iter=entry->entries().begin(); iter!=entry->entries().end(); iter++) {

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
      struct stat64 s, ls;
      stat64(tpath.c_str(),&s);
      stat64(talnk.c_str(),&ls);
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
	  _symlink(tlink.c_str(), tpath.c_str());
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
      _symlink(o.str().c_str(), tpath.c_str()); }
  }
}

// Checks that the key link exists and is up to date
bool Device::update_key_file(const string& config, const string& path)
{    
  const int line_size=128;
  char buff[line_size];
  bool outofdate=false;
  const TableEntry* entry = _table.get_top_entry(config);
  if (!entry) return false;

  outofdate |= _check_config_file(entry, path, entry->key());

  const TableEntry* gentry = _table.get_top_entry(string(GlobalCfg::name()));
  if (gentry)
    outofdate |= _check_config_file(gentry, path, entry->key());

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

    _make_config_file(entry, path, key);

    if (gentry)
      _make_config_file(gentry, path, key);

    TableEntry t(entry->name(), key, entry->entries());
    _table.set_top_entry(t);
  }
  return outofdate;
}


void Device::update_keys(const Path& path, XtcTable& xtc, time_t time_key)
{
#ifdef DBUG
  printf("device:%s %u\n",_name.c_str(),(unsigned)time_key);
#endif
  for(list<TableEntry>::iterator it=_table.entries().begin(); it!=_table.entries().end(); it++) {  // alias
    bool changed=false;
    for(list<FileEntry>::const_iterator fit=it->entries().begin(); fit!=it->entries().end(); fit++) {  // xtc files
      QTypeName qtype = PdsDefs::qtypeName(UTypeName(fit->name()));
      XtcTableEntry* xe = xtc.entry(qtype);
      if (!xe) {
#ifdef DBUG
        printf("no %s : create %s/%s.%u\n", qtype.c_str(), qtype.c_str(), fit->entry().c_str(), 0);
#endif
        changed = true;
        XtcFileEntry fe(fit->entry(),0,time(0));
        fe.update(path.data_path(qtype));
        xe = new XtcTableEntry(qtype);
        xe->entries().push_back(fe);
        xtc.entries().push_back(*xe);
        delete xe;
      }
      else {
        XtcFileEntry* fe = xe->entry(fit->entry());
        if (!fe) {
#ifdef DBUG
          printf("no %s/%s : create %s/%s.%u\n", 
                 qtype.c_str(), fit->entry().c_str(), qtype.c_str(), fit->entry().c_str(), 0);
#endif
          changed = true;
          fe = new XtcFileEntry(fit->entry(),0,time(0));
          fe->update(path.data_path(qtype));
          xe->entries().push_back(*fe);
          delete fe;
        }
        else if (fe->time() > time_key) {
#ifdef DBUG
          printf("update %u/%u : %s/%s.%u\n", (unsigned)time_key, (unsigned)fe->time(), qtype.c_str(), fit->entry().c_str(), fe->ext());
#endif
          changed = true;
          fe->update(path.data_path(qtype));
        }
        else {
#ifdef DBUG
          //          printf("valid %u/%u : %s/%s.%u\n", (unsigned)time_key, (unsigned)fe->time(), qtype.c_str(), fit->entry().c_str(), fe->ext());
#endif
          ;  // no change
        }
      }
    }
    
    if (changed) {
      it->update(_table._next_key);
      // make the key
      mode_t mode = _fmode;
      string kpath = path.key_path(_name, _table._next_key);
#ifdef DBUG
      printf("mkdir %s\n",kpath.c_str());
#endif
      mkdir(kpath.c_str(),mode);
      _table._next_key++;

      for(list<FileEntry>::const_iterator fit=it->entries().begin(); fit!=it->entries().end(); fit++) {  // xtc files
        UTypeName utype(fit->name());
        QTypeName qtype = PdsDefs::qtypeName(utype);
        XtcFileEntry* fe = xtc.entry(qtype)->entry(fit->entry());
        if (!fe) {
          printf("No file entry %s/%s\n",qtype.c_str(),fit->entry().c_str());
          continue;
        }

        char buff[16];
        sprintf(buff,"/%08x",PdsDefs::typeId(utype)->value());
        string tpath = kpath+buff;
        ostringstream o;
        o << "../../../xtc/" << qtype << "/" << fe->name() << "." << fe->ext();
#ifdef DBUG
        printf("symlink %s -> %s\n",tpath.c_str(), o.str().c_str());
#endif
        _symlink(o.str().c_str(),tpath.c_str());
      }
    }
  }
}


