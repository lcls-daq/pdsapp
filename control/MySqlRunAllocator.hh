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
    int      reportDetectors(int expt, int run, std::vector<std::string>& names);
    int      reportTotals(int expt, int run, long events, long damaged, double gigabytes);

  private:
    OfflineClient* _offlineclient;
  };

};

#endif
