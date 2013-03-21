#ifndef Pds_ConfigDb_Devices_hh
#define Pds_ConfigDb_Devices_hh

#include "pdsapp/config/DeviceEntry.hh"
#include "pdsapp/config/Table.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <time.h>
#include <list>
#include <string>

using std::list;
using std::string;

namespace Pds_ConfigDb {

  class UTypeName;
  class XtcTable;
  class Path;

  class Device {
  public:
    Device();
    Device( const string& path, const string& name,
	    const list<DeviceEntry>& src_list );
  public:
    void load(const char*&);
    void save(char*&) const;
  public:
    bool operator==(const Device&) const;
  public:
    const string& name() const { return _name; }

    //  Table of {device alias, key, {config_type,filename}}
    const Table&  table() const { return _table; }
    Table& table() { return _table; }

    //  List of Pds::Src entries
    const list<DeviceEntry>& src_list() const { return _src_list; }
    list<DeviceEntry>& src_list() { return _src_list; }
  public:
    string keypath (const string& path, const string& key) const;
    string xtcpath (const string& path, const UTypeName& uname, const string& entry) const;
    string typepath(const string& path, const string& key, const UTypeName& entry) const;
    string typelink(const UTypeName& name, const string& entry) const;
  public:
    void   update_keys (const Path&, XtcTable&, time_t);
  public:    // Deprecated
    bool   validate_key_file(const string& config, const string& path);
    bool   update_key_file  (const string& config, const string& path);
  private:
    bool _check_config_file(const TableEntry* entry, const string& path, const string& key);
    void _make_config_file (const TableEntry* entry, const string& path, const string& key);
  private:
    string _name;
    Table  _table;
    list<DeviceEntry> _src_list;
  };

};

#endif
