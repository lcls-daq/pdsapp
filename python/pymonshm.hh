#ifndef PdsPython_pymonshm_hh
#define PdsPython_pymonshm_hh

#include <string>
using std::string;

namespace Pds { 
  class MyMonitorServer; 
  class FrameProcessor;
}

typedef struct {
  PyObject_HEAD
  Pds::MyMonitorServer* srv;
  Pds::FrameProcessor*  processor;
} pdsmonshm;

#endif
