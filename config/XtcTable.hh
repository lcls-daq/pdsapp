#ifndef Pds_ConfigDb_XtcTable_hh
#define Pds_ConfigDb_XtcTable_hh

#include <list>
using std::list;
#include <string>
using std::string;

#include <sys/stat.h>

namespace Pds_ConfigDb {

  class Path;

  class XtcFileEntry {
  public:
    XtcFileEntry();
    XtcFileEntry(const string& name, unsigned ext, time_t time);
  public:
    void load(const char*&);
    void save(char*&) const;
  public:
    string   name () const { return _name; }
    unsigned ext  () const { return _ext;  }
    time_t   time () const { return _time; }
  public:
    void     time    (time_t t) { _time=t; }
    void     update (const string& path);
    bool     updated() const { return _updated; }
  private:
    string   _name;
    unsigned _ext;
    time_t   _time;
    mutable bool     _updated;
  };

  class XtcTableEntry { 
  public:
    XtcTableEntry();
    XtcTableEntry(const string& name);
  public:
    void load(const char*&);
    void save(char*&) const;
  public:
    string   name () const { return _name; }
    list<XtcFileEntry>&       entries()       { return _entries; }    
    const list<XtcFileEntry>& entries() const { return _entries; }
    XtcFileEntry*             entry  (const string&);
  private:
    string _name;
    list<XtcFileEntry> _entries;
  };

  class XtcTable {
  public:
    XtcTable(const string&);
  public:
    void load(const char*&);
    void save(char*&) const;
    void write(const string& path) const;
    void read (const string& path);
  public:
    enum Result { Valid, OutOfDate, NonExistent };
    Result check(const string& type, const string& name, time_t time) const;
    void   update(const Path&, time_t);
    void   update_xtc(const string& type, const string& name);
    void   dump  () const;
  public:
    list<XtcTableEntry>&       entries()       { return _entries; }
    const list<XtcTableEntry>& entries() const { return _entries; }
    XtcTableEntry*             entry  (const string&);
  private:
    void _construct(const string&);
  private:
    list<XtcTableEntry> _entries;
  };
};

#endif
