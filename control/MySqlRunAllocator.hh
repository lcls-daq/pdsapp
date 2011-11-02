#ifndef Pds_MySqlRunAllocator_hh
#define Pds_MySqlRunAllocator_hh

#include <string>
#include "pds/management/RunAllocator.hh"

namespace Pds {

  class OfflineClient; 

  class MySqlRunAllocator : public RunAllocator {
  public:
    MySqlRunAllocator(OfflineClient*);
    unsigned alloc();
    int      reportOpenFile(int expt, int run, int stream, int chunk, std::string& host, std::string& fname);
  private:
    OfflineClient* _offlineclient;
  };

};

#endif
