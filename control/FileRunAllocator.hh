#ifndef Pds_FileRunAllocator_hh
#define Pds_FileRunAllocator_hh

#include "pds/management/RunAllocator.hh"

namespace Pds {

  class OfflineClient; 

  class FileRunAllocator : public RunAllocator {
  public:
    FileRunAllocator(const char*);
    unsigned alloc();
  private:
    const char* _runNumberPath;
  };

};

#endif
