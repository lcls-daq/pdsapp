#ifndef Pds_WSRunAllocator_hh
#define Pds_WSRunAllocator_hh

#include <string>
#include "pds/management/RunAllocator.hh"

namespace Pds {

  class OfflineClient;

  class WSRunAllocator : public RunAllocator {
  public:
    WSRunAllocator(OfflineClient*);
    unsigned beginRun();
    unsigned endRun();
    int reportOpenFile(int, int, int, std::string&, std::string&);
    int reportDetectors(int, std::vector<std::string>&);
    int reportTotals(int, long, long, double);

  private:
    OfflineClient* _offlineclient;
  };

};

#endif
