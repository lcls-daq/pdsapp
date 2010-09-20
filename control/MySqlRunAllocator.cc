
#include "MySqlRunAllocator.hh"
#include "pds/offlineclient/OfflineClient.hh"
#include <stdio.h>

using namespace Pds;

MySqlRunAllocator::MySqlRunAllocator(OfflineClient* oc) :
  _offlineclient(oc) {
}

unsigned MySqlRunAllocator::alloc() {
  // be careful, _offlineclient could be NULL
  if (_offlineclient) {
    unsigned int runNumber = 0;
    if (_offlineclient->AllocateRunNumber(&runNumber)) {
      printf("Error: AllocateRunNumber failed in StateSelect\n");
      //      _w.log().append("Error: AllocateRunNumber failed in StateSelect\n");
      return (unsigned)Error;
    } else {
      return runNumber;
    }
  } else {
    // no offline client -- set run number to 0
    return 0;
  }
};

int MySqlRunAllocator::reportOpenFile(int expt, int run, int stream, int chunk)
{
  return _offlineclient ? _offlineclient->reportOpenFile(expt,run,stream,chunk) : 0;
}
