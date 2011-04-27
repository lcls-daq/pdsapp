#ifndef PdsPython_pydaq_hh
#define PdsPython_pydaq_hh

#include <string>
using std::string;

typedef struct {
  PyObject_HEAD
  int     socket;
  char*   dbpath;
  int32_t dbkey;
  char*   buffer;
  int32_t runinfo;
} pdsdaq;

#endif
