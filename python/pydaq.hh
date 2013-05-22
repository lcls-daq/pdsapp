#ifndef PdsPython_pydaq_hh
#define PdsPython_pydaq_hh

#include <string>
using std::string;

namespace Pds { class RemotePartition; }

typedef struct {
  PyObject_HEAD
  unsigned addr;
  unsigned platform;
  int      socket;
  int      state;
  char*    dbpath;
  char*    dbalias;
  int32_t  dbkey;
  char*    buffer;
  int32_t  runinfo;
  Pds::RemotePartition* partition;
} pdsdaq;

#endif
