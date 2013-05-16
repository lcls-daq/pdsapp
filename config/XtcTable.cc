#include "pdsapp/config/XtcTable.hh"
#include "pdsapp/config/Path.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/XML.hh"

#include <sys/stat.h>

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

#include <glob.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBUG

using namespace Pds_ConfigDb;

XtcFileEntry::XtcFileEntry() : _name("None"), _ext(0), _time(0), _updated(false) {}

XtcFileEntry::XtcFileEntry(const string& name,
                           unsigned      ext,
                           time_t        time) :
  _name(name), _ext(ext), _time(time), _updated(false) 
{
}

void XtcFileEntry::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "_name")
      _name  = XML::IO::extract_s(p);
    else if (tag.name == "_ext")
      _ext   = XML::IO::extract_i(p);
    else if (tag.name == "_time")
      _time  = XML::IO::extract_i(p);
  XML_iterate_close(XtcFileEntry,tag);
}

void XtcFileEntry::save(char*& p) const
{
  _updated = false;
  XML_insert(p, "string", "_name" , XML::IO::insert(p,_name));
  XML_insert(p, "int"   , "_ext"  , XML::IO::insert(p,_ext));
  XML_insert(p, "int"   , "_time" , XML::IO::insert(p,(unsigned)_time));
}

void XtcFileEntry::update(const string& path)
{
  if (!_updated) {
#ifdef DBUG
    printf("update %s/%s\n",path.c_str(),_name.c_str());
#endif
    ostringstream o;
    o << "cp " << path << "/" << _name
      << " "   << path << "/" << _name << '.' << ++_ext;
    system(o.str().c_str());
    _updated = true;
  }
}

XtcTableEntry::XtcTableEntry() : _name("None") {}

XtcTableEntry::XtcTableEntry(const string& name) : _name(name) {}

void XtcTableEntry::load(const char*& p)
{
  _entries.clear();

  XML_iterate_open(p,tag)
    if      (tag.name == "_name")
      _name = XML::IO::extract_s(p);
    else if (tag.name == "_entries") {
      XtcFileEntry e;
      e.load(p);
      _entries.push_back(e);
    }
  XML_iterate_close(XtcTableEntry,tag);
}

void XtcTableEntry::save(char*& p) const
{
  XML_insert(p, "string", "_name", XML::IO::insert(p,_name));
  for(list<XtcFileEntry>::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    XML_insert(p, "XtcFileEntry", "_entries", it->save(p) );
  }
}

XtcFileEntry* XtcTableEntry::entry(const string& name)
{
  for(list<XtcFileEntry>::iterator it=_entries.begin(); it!=_entries.end(); it++) {
    XtcFileEntry& e = *it;
    if (e.name() == name) return &e;
  }
  return 0;
}

XtcTable::XtcTable(const string& path)
{
  string fname = path + "/db/xtc.xml";
  FILE* f = fopen(fname.c_str(),"r");
  if (!f) {
    perror("fopen(r) xtc.xml");
    read(path+"/xtc");
  }
  else {
    struct stat64 s;
    if (fstat64(fileno(f),&s))
      perror("fstat64 xtc.xml");
    else {
      const char* TRAILER = "</Document>";
      char* buff = new char[s.st_size+strlen(TRAILER)+1];
      if (fread(buff, 1, s.st_size, f) != size_t(s.st_size))
        perror("fread expt.xml");
      else {
        strcpy(buff+s.st_size,TRAILER);
        const char* p = buff;
        load(p);
      }
      delete[] buff;
    }
    fclose(f);
  }
}

void XtcTable::read(const string& path)
{
#ifdef DBUGV
  printf("XtcTable::read %s\n",path.c_str());
#endif
  glob_t g;
  string gpath = path + "/[a-f,A-F]*";
  glob(gpath.c_str(),0,0,&g);
  for(unsigned i=0; i<g.gl_pathc; i++) {
#ifdef DBUGV
    printf("\tqtype %s(%s)\n",basename(g.gl_pathv[i]),g.gl_pathv[i]);
#endif
    XtcTableEntry te(basename(g.gl_pathv[i]));
    glob_t gg;
    string ggpath(g.gl_pathv[i]);
    ggpath += "/*.xtc";
    glob(ggpath.c_str(),0,0,&gg);
    for(unsigned j=0; j<gg.gl_pathc; j++) {
#ifdef DBUGV
      printf("\t\tfile %s(%s)\n",basename(gg.gl_pathv[j]),gg.gl_pathv[j]);
#endif
      struct stat64 s;
      stat64(gg.gl_pathv[j],&s);
      glob_t ggg;
      string gggpath(gg.gl_pathv[j]);
      gggpath += ".*";
      glob(gggpath.c_str(),0,0,&ggg);
#ifdef DBUGV
      printf("\t\tfound %d %s.*\n",int(ggg.gl_pathc),gg.gl_pathv[j]);
#endif
      int n = ggg.gl_pathc;
      if (!n) {
        ostringstream o;
        o << "cp " << gg.gl_pathv[j] << " " << gg.gl_pathv[j] << ".0";
        system(o.str().c_str());
        n = 1;
      }
      te.entries().push_back(XtcFileEntry(basename(gg.gl_pathv[j]),n-1,s.st_mtime));
      globfree(&ggg);
    }
    globfree(&gg);
    entries().push_back(te);
  }
  globfree(&g);
}

void XtcTable::write(const string& path) const
{
  string fname = path + "/db/xtc.xml";
  FILE* f = fopen(fname.c_str(),"w");
  if (!f)
    perror("fopen(w) xtc.xml");
  else {
    const int MAX_SIZE = 0x100000;
    char* buff = new char[MAX_SIZE];
    char* p = buff;

    save(p);

    if (p > buff+MAX_SIZE) {
      perror("save overwrites array");
    } 
    else 
      fwrite(buff, 1, p-buff, f);

    delete[] buff;
    fclose(f);
  }
}

void XtcTable::load(const char*& p)
{
  _entries.clear();

  XML_iterate_open(p,tag)
    if (tag.name == "_entries") {
      XtcTableEntry e;
      e.load(p);
      _entries.push_back(e);
    }
  XML_iterate_close(XtcTable,tag);
}

void XtcTable::save(char*& p) const
{
  for(list<XtcTableEntry>::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    XML_insert(p, "XtcTableEntry", "_entries", it->save(p) );
  }
}

void XtcTable::update(const Path& path, time_t time_key)
{
  for(list<XtcTableEntry>::iterator it=_entries.begin(); it!=_entries.end(); it++) {
    XtcTableEntry& e = *it;
    for(list<XtcFileEntry>::iterator fit=e.entries().begin(); fit!=e.entries().end(); fit++) {
      if (fit->time() > time_key && !fit->updated())
        fit->update(path.data_path(QTypeName(e.name())));
    }
  }
}

XtcTableEntry* XtcTable::entry(const string& name)
{
  for(list<XtcTableEntry>::iterator it=_entries.begin(); it!=_entries.end(); it++) {
    XtcTableEntry& e = *it;
    if (e.name() == name) return &e;
  }
  return 0;
}

void XtcTable::dump() const
{
  printf("=== XtcTable ===\n");
  for(list<XtcTableEntry>::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    const XtcTableEntry& e = *it;
    printf("\t%s\n",e.name().c_str());
    for(list<XtcFileEntry>::const_iterator fit=e.entries().begin(); fit!=e.entries().end(); fit++) {
      printf("\t\t%s %u %u %s\n",
             fit->name().c_str(), fit->ext(), unsigned(fit->time()),
             fit->updated() ? "changed" : "unchanged");
    }
  }
}

void XtcTable::update_xtc(const string& type, const string& name)
{
  XtcTableEntry* te = entry(type);
  if (!te) {
    _entries.push_back(XtcTableEntry(type));
    te = entry(type);
  }

  XtcFileEntry* fe = te->entry(name);
  if (fe)
    *fe = XtcFileEntry(name,fe->ext(), time(0));
  else
    te->entries().push_back(XtcFileEntry(name, 0, time(0)));
}
