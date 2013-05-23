#include "pdsapp/config/Table.hh"
#include "pdsapp/config/XML.hh"

#include <sys/stat.h>

#include <fstream>
using std::ifstream;
using std::ofstream;

#include <iomanip>
#include <sstream>

using std::ostringstream;
using std::hex;
using std::setw;
using std::setfill;

using namespace Pds_ConfigDb;

const int line_size=128;

static list<FileEntry> _read_table(const string& path)
{
  list<FileEntry> l;
  ifstream f(path.c_str());
  for(FileEntry e(f); f.good(); e=FileEntry(f))
    l.push_back(e);
  l.sort();
  return l;
}

static void _write_table(const list<FileEntry>& table, const string& path)
{
  ofstream f(path.c_str());
  for(list<FileEntry>::const_iterator iter = table.begin(); iter != table.end(); iter++)
    f << iter->name() << '\t' << iter->entry() << std::endl;
}

//===============
//  FileEntry 
//===============

FileEntry::FileEntry() {}

FileEntry::FileEntry(istream& i) 
{
  i >> _name >> _entry;
}

FileEntry::FileEntry(const string& name, const string& entry) :
  _name(name), _entry(entry) {}

void FileEntry::load(const char*& p)
{
  XML_iterate_open(p,tag)
    if      (tag.name == "_name")
      _name  = XML::IO::extract_s(p);
    else if (tag.name == "_entry")
      _entry = XML::IO::extract_s(p);
  XML_iterate_close(FileEntry,tag);
}

void FileEntry::save(char*& p) const
{
  XML_insert(p, "string", "_name" , XML::IO::insert(p,_name));
  XML_insert(p, "string", "_entry", XML::IO::insert(p,_entry));
}

bool FileEntry::operator==(const FileEntry& e) const
{
  return name()==e.name();
}

bool FileEntry::operator< (const FileEntry& e) const
{
  return name()<e.name();
}

void FileEntry::read(istream& i)
{
  i >> _name >> _entry;
}

string FileEntry::name() const
{
  return _name;
}

//===============
//  TableEntry 
//===============

TableEntry::TableEntry() : _name("None"), _key("Unassigned"), _changed(false) {}

TableEntry::TableEntry(const string& name) : _name(name), _key("Unassigned"), _changed(false) {}

TableEntry::TableEntry(const string& name, const string& key,
		       const FileEntry& entry) :
  _name(name), _key(key), _changed(false)
{ _entries.push_back(entry); }

TableEntry::TableEntry(const string& name, const string& key,
		       const list<FileEntry>& entries) :
  _name(name), _key(key), _entries(entries), _changed(false)
{}

void TableEntry::load(const char*& p)
{
  _entries.clear();

  XML_iterate_open(p,tag)
    if      (tag.name == "_name")
      _name = XML::IO::extract_s(p);
    else if (tag.name == "_key")
      _key  = XML::IO::extract_s(p);
    else if (tag.name == "_entries") {
      FileEntry e;
      e.load(p);
      _entries.push_back(e);
    }
  XML_iterate_close(TableEntry,tag);

  _changed = false;
}

void TableEntry::save(char*& p) const
{
  XML_insert(p, "string", "_name", XML::IO::insert(p,_name));
  XML_insert(p, "string", "_key" , XML::IO::insert(p,_key));
  for(list<FileEntry>::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    XML_insert(p, "FileEntry", "_entries", it->save(p) );
  }
  _changed = false;
}

bool TableEntry::operator==(const TableEntry& e) const
{
  return _name == e._name;
}

void TableEntry::set_entry(const FileEntry& e)
{
  _entries.remove(e);
  _entries.push_back(e);
}

void TableEntry::remove(const FileEntry& e)
{
  _entries.remove(e);
}

void TableEntry::update(unsigned key) 
{
  ostringstream o;
  o << hex << setw(8) << setfill('0') << key;
  _key = o.str();
  _changed = true;
}

bool TableEntry::updated() const { return _changed; }

//===============
//  Table 
//===============

Table::Table() : _next_key(0)
{
}

Table::Table(const string& path) : _next_key(0)
{
  struct stat64 s;
  if (!stat64(path.c_str(),&s)) {
    char buff[line_size];
    list<FileEntry> top(_read_table(path));
    for(list<FileEntry>::iterator iter = top.begin(); iter != top.end(); iter++) {
      sprintf(buff,"%s.%s",path.c_str(),iter->name().c_str());
      _entries.push_back(TableEntry(iter->name(),iter->entry(),_read_table(buff)));
    }
  }
}

void Table::load(const char*& p)
{
  _entries.clear();

  XML_iterate_open(p,tag)
    if      (tag.element == "TableEntry") {
      TableEntry e;
      e.load(p);
      _entries.push_back(e);
    }
    else if (tag.name == "_next_key")
      _next_key = XML::IO::extract_i(p);
  XML_iterate_close(Table,tag);
}

void Table::save(char*& p) const
{
  XML_insert(p, "unsigned", "_next_key", XML::IO::insert(p, _next_key) );
  for(list<TableEntry>::const_iterator it=_entries.begin(); it!=_entries.end(); it++) {
    XML_insert(p, "TableEntry", "_entries", it->save(p) );
  }
}

void Table::write(const string& path) const
{
  list<FileEntry> top;
  for(list<TableEntry>::const_iterator iter=_entries.begin(); iter!=_entries.end(); iter++)
    top.push_back(FileEntry(iter->name(),iter->key()));
  _write_table(top,path);

  for(list<TableEntry>::const_iterator iter=_entries.begin(); iter!=_entries.end(); iter++) {
    string p = path + "." + iter->name();
    _write_table(iter->entries(),p);
  }
}
 
void Table::dump(const string& path) const
{
  printf("\nCfgTable %s\n",path.c_str());
  for(list<TableEntry>::const_iterator iter=_entries.begin(); iter!=_entries.end(); iter++) {
    printf("%s\t%s\n",iter->name().c_str(),iter->key().c_str());
    for(list<FileEntry>::const_iterator fiter=iter->entries().begin();
	fiter!=iter->entries().end(); fiter++)
      printf("[%s,%s]\t",fiter->name().c_str(),fiter->entry().c_str());
    printf("\n");
  }
}

//  Return a copied list of strings of the names of each entry
list<string> Table::get_top_names() const
{
  list<string> l;
  for(list<TableEntry>::const_iterator iter=_entries.begin(); iter!=_entries.end(); iter++)
    l.push_back(iter->name());
  return l;
}

const TableEntry* Table::get_top_entry(const string& name) const
{
  for(list<TableEntry>::const_iterator iter=_entries.begin(); iter!=_entries.end(); iter++)
    if (iter->name()==name)
      return &(*iter);
  return 0;
}

void Table::set_top_entry(const TableEntry& e)
{
  for(list<TableEntry>::iterator iter=_entries.begin(); iter!=_entries.end(); iter++)
    if (iter->name()==e.name()) {
      *iter = e;
      break;
    }
}

void Table::new_top_entry(const string& name)
{
  _entries.push_back(TableEntry(name));
}

void Table::remove_top_entry(const TableEntry& e)
{
  _entries.remove(e);
}

void Table::copy_top_entry(const string& dst, const string& src)
{
  for(list<TableEntry>::iterator iter=_entries.begin(); iter!=_entries.end(); iter++)
    if (iter->name()==src) {
      _entries.push_back(TableEntry(dst,"Unassigned",iter->entries())); 
      break;
    }
}

void Table::set_entry(const string& top, const FileEntry& e)
{
  TableEntry* entry = const_cast<TableEntry*>(get_top_entry(top));
  if (entry)
    entry->set_entry(e);
  else {
    TableEntry te(top,"Unassigned",e);
    _entries.push_back(te);
  }
}

void Table::clear_entry(const string& top, const FileEntry& e)
{
  TableEntry* entry = const_cast<TableEntry*>(get_top_entry(top));
  if (entry)
    entry->remove(e);
}

