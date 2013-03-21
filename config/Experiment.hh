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
    void load(const char*&);
    void save(char*&) const;
  public:
    void create();
    void read();
    void write() const;
    Experiment* branch(const string&) const;
  public:
    void read_file();
    void write_file() const;
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
    void dump() const;

  public:
    void     update_keys();
    unsigned next_key   () const;
  public:
    bool     update_key_file(const TableEntry&);
    unsigned next_key_file() const;

    static void log_threshold(double);
  private:
    Path _path;
    Table  _table;
    list<Device> _devices;
    mutable time_t   _time_db;
    time_t   _time_key;
  };
};

#endif
