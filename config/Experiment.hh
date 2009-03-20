#ifndef Pds_ConfigDb_Experiment_hh
#define Pds_ConfigDb_Experiment_hh

#include "pdsapp/config/Table.hh"
#include "pdsapp/config/Device.hh"

#include <string>
using std::string;
#include <list>
using std::list;

namespace Pds_ConfigDb {

  class Experiment {
  public:
    Experiment(const string&);
  public:
    bool is_valid() const;
    void create();
    void read();
    void write() const;
  public:
    const Table& table() const { return _table; }
    Table& table() { return _table; }
    const list<Device>& devices() const { return _devices; }
    list<Device>& devices() { return _devices; }
    Device* device(const string&);
  public:
    void add_device(const string&, const list<DeviceEntry>&);
    string data_path(const string& device, const string& type) const;
    string desc_path(const string& device, const string& type) const;
    list<string> xtc_files(const string& device, const string& type) const;
    string key_path (const string& device, const string& key ) const;
    void import_data(const string& device,
		     const string& type,
		     const string& file,
		     const string& desc);
    void update_keys();

    void dump() const;
  private:
    bool validate_key(const TableEntry&);
  private:
    string _path;
    Table  _table;
    list<Device> _devices;
  };
};

#endif
