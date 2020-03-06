#ifndef PdsPython_pydaq_hh
#define PdsPython_pydaq_hh

#ifdef WITH_THREAD
#include "pythread.h"
#endif
#include <string>

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
  bool     record;
  char*    buffer;
  char*    exptname;
  int32_t  runnum;
  Pds::RemotePartition* partition;
  int      waiting;
  int      pending;
  int      signal[2];
#ifdef WITH_THREAD
  bool     blocking;
  PyThread_type_lock lock;
#endif
} pdsdaq;

#endif
