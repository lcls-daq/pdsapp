#ifndef Pds_ConfigDb_Device_File_hh
#define Pds_ConfigDb_Device_File_hh

#include "pdsapp/config/DeviceEntry.hh"
#include "pdsapp/config/Table_File.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <list>
#include <string>

using std::list;
using std::string;

namespace Pds_ConfigDb {

  class UTypeName;

  namespace File {
    class Device {
    public:
      Device( const string& path, const string& name,
              const list<DeviceEntry>& src_list );
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
      string keypath (const string& path, const string& key);
      string xtcpath (const string& path, const UTypeName& uname, const string& entry) const;
      string typepath(const string& path, const string& key, const UTypeName& entry);
      string typelink(const UTypeName& name, const string& entry);
      bool   validate_key(const string& config, const string& path);
      bool   update_key  (const string& config, const string& path);
    private:
      bool _check_config(const TableEntry* entry, const string& path, const string& key);
      void _make_config (const TableEntry* entry, const string& path, const string& key);
    private:
      string _name;
      Table  _table;
      list<DeviceEntry> _src_list;
    };
  };
};

#endif
