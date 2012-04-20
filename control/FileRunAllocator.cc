
#include "FileRunAllocator.hh"
#include <stdio.h>

using namespace Pds;

FileRunAllocator::FileRunAllocator(const char* runNumberPath) :
  _runNumberPath(runNumberPath) {
}

unsigned FileRunAllocator::alloc() {
  unsigned int runNumber = 0;   // default run number is 0
  // be careful, _runNumberPath could be NULL
  if (_runNumberPath) {
    unsigned int runTmp = 0;
    FILE *runFile = fopen(_runNumberPath, "r+");
    if (!runFile) {
      perror("fopen");
      fprintf(stderr, "Error: failed to open run number file '%s'.\n", _runNumberPath);
    } else if (fscanf(runFile, "%u", &runTmp) != 1) {
      fprintf(stderr, "Error: failed to read run number from file '%s'.\n", _runNumberPath);
    } else {
      runNumber = runTmp;
      rewind(runFile);    // seek to beginning of file
      ++runNumber;
      if (fprintf(runFile, "%u\n", runNumber) <= 0) {
        perror("fprintf");
        fprintf(stderr, "Error: failed to write run number %d to file '%s'.\n", runNumber, _runNumberPath);
      }
    }
    // clean up
    if (runFile) {
      fclose(runFile);
    }
  }
  if (runNumber == 0) {
    fprintf(stderr, "Warning: %s is returning 0\n", __PRETTY_FUNCTION__);
  }
  return (runNumber);
};
