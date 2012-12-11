#ifndef Pds_ConfigDb_Table_hh
#define Pds_ConfigDb_Table_hh

#include <list>
using std::list;
#include <string>
using std::string;
#include <iostream>
using std::istream;

namespace Pds_ConfigDb {

  class FileEntry {
  public:
    FileEntry();
    FileEntry(istream&);
    FileEntry(const string& name, const string& entry);
  public:
    string name () const;
    const string& entry() const { return _entry; }
  public:
    bool operator==(const FileEntry&) const;
    bool operator< (const FileEntry&) const;
    void read(istream&);
  private:
    string _name;
    string _entry;
  };

  class TableEntry {
  public:
    TableEntry(const string& name);
    TableEntry(const string& name, const string& key,
	       const FileEntry& entry);
    TableEntry(const string& name, const string& key,
	       const list<FileEntry>& entries);
  public:
    const string& name() const { return _name; }
    const string& key () const { return _key; }
    const list<FileEntry>& entries() const { return _entries; }
    bool  operator==(const TableEntry&) const;
  public:
    void set_entry(const FileEntry& entry);
    void remove   (const FileEntry& entry);
  private:
    string _name;
    string _key;
    list<FileEntry> _entries;
  };

  class Table {
  public:
    Table();
    Table(const string& path);
  public:
    const std::list<TableEntry>& entries() const { return _entries; }
    std::list<TableEntry>& entries() { return _entries; }

    std::list<string> get_top_names() const;
    const TableEntry* get_top_entry(const string&) const;
    
    void write(const string&) const;
    void set_top_entry(const TableEntry&);
    void new_top_entry(const string&);
    void remove_top_entry(const TableEntry&);
    void copy_top_entry(const string&,const string&);
    void set_entry(const string&, const FileEntry&);
    void clear_entry(const string&, const FileEntry&);

    void dump(const string&) const;
  private:
    list<TableEntry> _entries;
  };
};

#endif

