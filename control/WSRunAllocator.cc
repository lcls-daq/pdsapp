#include "pds/offlineclient/OfflineClient.hh"
#include "WSRunAllocator.hh"
#include <stdio.h>

using namespace Pds;

WSRunAllocator::WSRunAllocator(OfflineClient* oc) :
  _offlineclient(oc) {
}

unsigned WSRunAllocator::beginRun() {
  // be careful, _offlineclient could be NULL
  if (_offlineclient) {
    unsigned int runNumber = 0;
    if (_offlineclient->BeginNewRun(&runNumber)) {
        printf("Error: BeginNewRun failed in StateSelect\n");
    }
    return runNumber;
  } else {
    // no offline client -- set run number to 0
    return 0;
  }
};

unsigned WSRunAllocator::endRun() {
  return _offlineclient ? _offlineclient->EndCurrentRun() : 0;
}

int WSRunAllocator::reportOpenFile(int run, int stream, int chunk, std::string& host, std::string& fname) {
  return _offlineclient ? _offlineclient->reportOpenFile(fname, run,stream,chunk,host) : 0;
}

int WSRunAllocator::reportDetectors(int run, std::vector<std::string>& names) {
  return _offlineclient ? _offlineclient->reportDetectors(run,names) : 0;
}

int WSRunAllocator::reportTotals(int run, long events, long damaged, double gigabytes) {
  return _offlineclient ? _offlineclient->reportTotals(run, events, damaged, gigabytes) : 0;
}
