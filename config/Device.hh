#ifndef Pds_ConfigDb_Devices_hh
#define Pds_ConfigDb_Devices_hh

#include "pdsapp/config/Table.hh"
#include "pds/config/DeviceEntry.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <time.h>
#include <list>
#include <string>

using std::list;
using std::string;

namespace Pds_ConfigDb {

  class UTypeName;

  class Device {
  public:
    Device();
    Device( const string& name,
            const Table& table, 
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
  private:
    string _name;
    Table  _table;
    list<DeviceEntry> _src_list;
  };

};

#endif
