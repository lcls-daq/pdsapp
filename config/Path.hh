#ifndef Pds_ConfigPath_hh
#define Pds_ConfigPath_hh

#include <string>
#include <list>
using std::string;
using std::list;

namespace Pds_ConfigDb {

  class UTypeName;
  class QTypeName;

  class Path {
  public:
    Path(const string&);
    ~Path();
  public:
    const string& base() const { return _path; }
    string expt() const;
    string devices() const;
    string device (const string&) const;
    string xtc() const;

    string data_path(const string& device, const UTypeName& type) const;
    string data_path(const QTypeName& type) const;
    string desc_path(const string& device, const UTypeName& type) const;
    list<string> xtc_files(const string& device, const UTypeName& type) const;
    //  Device key
    string key_path (const string& device, const string& key ) const;
    string key_path (const string& device, unsigned key ) const;
    //  Global key
    string key_path (const string& key) const;
    string key_path (unsigned key) const;

    bool   is_valid() const;
    void   create();
  private:
    string _path;
  };

};

#endif
