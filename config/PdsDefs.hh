#ifndef Pds_ConfigDb_PdsDefs_hh
#define Pds_ConfigDb_PdsDefs_hh

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <string>
using std::string;

namespace Pds_ConfigDb {
  class PdsDefs {
  public:
    static void initialize();

    static const string& type_id(unsigned);
    static unsigned type_index(const string&);

    static const string& detector(unsigned);
    static const string& device  (unsigned);
  };
};

#endif
