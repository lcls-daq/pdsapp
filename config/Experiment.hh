#ifndef Pds_ConfigDb_Experiment_hh
#define Pds_ConfigDb_Experiment_hh

#include "pdsapp/config/Path.hh"
#include "pdsapp/config/Table.hh"
#include "pdsapp/config/Device.hh"

#include <string>
using std::string;
#include <list>
using std::list;

namespace Pds_ConfigDb {

  class UTypeName;

  class Experiment {
  public:
    Experiment(const Path&);
  public:
    void create();
    void read();
    void write() const;
    Experiment* branch(const string&) const;
  public:
    const Path& path() const { return _path; }
    const Table& table() const { return _table; }
    Table& table() { return _table; }
    const list<Device>& devices() const { return _devices; }
    list<Device>& devices() { return _devices; }
    Device* device(const string&);
    int  current_key(const string&) const;
  public:
    void add_device(const string&, const list<DeviceEntry>&);
    void remove_device(const Device&);
    void import_data(const string& device,
		     const UTypeName& type,
		     const string& file,
		     const string& desc);
    void update_keys();

    void dump() const;
  public:
    bool update_key(const TableEntry&);
    unsigned next_key() const;
  private:
    Path _path;
    Table  _table;
    list<Device> _devices;
  };
};

#endif
