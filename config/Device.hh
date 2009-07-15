#ifndef Pds_ConfigDb_Devices_hh
#define Pds_ConfigDb_Devices_hh

#include "pdsapp/config/Table.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <list>
#include <string>

using std::list;
using std::string;

namespace Pds_ConfigDb {

  class UTypeName;

  class DeviceEntry : public Pds::Src {
  public:
    DeviceEntry(unsigned id);
    DeviceEntry(const Pds::Src& id);
    DeviceEntry(const string& id);
  public:
    string   id () const;
  };

  class Device {
  public:
    Device( const string& path, const string& name,
	    const list<DeviceEntry>& src_list );
  public:
    const string& name() const { return _name; }
    const Table&  table() const { return _table; }
    Table& table() { return _table; }
    const list<DeviceEntry>& src_list() const { return _src_list; }
    list<DeviceEntry>& src_list() { return _src_list; }
  public:
    string keypath (const string& path, const string& key);
    string typepath(const string& path, const string& key, const UTypeName& entry);
    string typelink(const UTypeName& name, const string& entry);
    bool   validate_key(const string& config, const string& path);
    bool   update_key  (const string& config, const string& path);
  private:
    string _name;
    Table  _table;
    list<DeviceEntry> _src_list;
  };

};

#endif
