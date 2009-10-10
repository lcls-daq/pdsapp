#ifndef Pds_MySqlRunAllocator_hh
#define Pds_MySqlRunAllocator_hh

#include "pds/management/RunAllocator.hh"

namespace Pds {

  class OfflineClient; 

  class MySqlRunAllocator : public RunAllocator {
  public:
    MySqlRunAllocator(OfflineClient*);
    unsigned alloc();
  private:
    OfflineClient* _offlineclient;
  };

};

#endif
