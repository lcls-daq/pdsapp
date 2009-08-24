#ifndef Pds_ConfigPath_hh
#define Pds_ConfigPath_hh

#include <string>
#include <list>
using std::string;
using std::list;

namespace Pds_ConfigDb {

  class UTypeName;

  class Path {
  public:
    Path(const string&);
    ~Path();
  public:
    const string& base() const { return _path; }
    string expt() const;
    string devices() const;
    string device (const string&) const;

    string data_path(const string& device, const UTypeName& type) const;
    string desc_path(const string& device, const UTypeName& type) const;
    list<string> xtc_files(const string& device, const UTypeName& type) const;
    string key_path (const string& device, const string& key ) const;
    string key_path (const string& key) const;

    bool   is_valid() const;
    void   create();
  private:
    string _path;
  };

};

#endif
