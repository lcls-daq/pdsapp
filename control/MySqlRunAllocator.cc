
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
      if (_offlineclient->BeginNewRun(runNumber)) {
        printf("Error: BeginNewRun failed in StateSelect\n");
      }
      return runNumber;
    }
  } else {
    // no offline client -- set run number to 0
    return 0;
  }
};

int MySqlRunAllocator::reportOpenFile(int expt, int run, int stream, int chunk, std::string& host, std::string& fname) {
  return _offlineclient ? _offlineclient->reportOpenFile(expt,run,stream,chunk,host,fname) : 0;
}

int MySqlRunAllocator::reportDetectors(int expt, int run, std::vector<std::string>& names) {
  return _offlineclient ? _offlineclient->reportDetectors(expt,run,names) : 0;
}

