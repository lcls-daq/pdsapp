#ifndef Pds_ConfigDb_Devices_hh
#define Pds_ConfigDb_Devices_hh

#include "pdsapp/config/Table.hh"
#include "pds/config/DeviceEntry.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <time.h>
#include <list>
#include <string>

namespace Pds_ConfigDb {

  class UTypeName;

  class Device {
  public:
    Device();
    Device( const std::string& name,
            const Table& table, 
	    const std::list<DeviceEntry>& src_list );
  public:
    bool operator==(const Device&) const;
  public:
    const std::string& name() const { return _name; }

    //  Table of {device alias, key, {config_type,filename}}
    const Table&  table() const { return _table; }
    Table& table() { return _table; }

    //  List of Pds::Src entries
    const std::list<DeviceEntry>& src_list() const { return _src_list; }
    std::list<DeviceEntry>& src_list() { return _src_list; }
  private:
    std::string _name;
    Table  _table;
    std::list<DeviceEntry> _src_list;
  };

};

#endif
