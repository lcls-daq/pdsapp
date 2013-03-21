#ifndef Pds_ConfigDb_DeviceEntry_hh
#define Pds_ConfigDb_DeviceEntry_hh

#include "pdsdata/xtc/DetInfo.hh"

#include <string>
using std::string;

namespace Pds_ConfigDb {

  class DeviceEntry : public Pds::Src {
  public:
    DeviceEntry(unsigned id=0);
    DeviceEntry(const Pds::Src& id);
    DeviceEntry(const string& id);
  public:
    void load(const char*&);
    void save(char*&) const;
  public:
    string   id  () const;
    string   path() const;
  };

};

#endif
