#ifndef Pds_PipeStream_hh
#define Pds_PipeStream_hh

//
// stream to attach idle appliances
//

#include "pds/utility/Stream.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/Src.hh"

namespace Pds {

  class Task;
  class InletWire;

  class PipeStream : public Stream {
  public:
    PipeStream(const Src& src, int read_fd);
    ~PipeStream();
  public:
    void       set_inlet_wire(InletWire*);
  public:
    void       start  ();
  private:
    void       _read  (char*&, int);
  private:
    int           _read_fd;
    Task*         _task;  // handle this stream in its own thread
    GenericPool   _pool;
    Src           _src;
    InletWire*    _wire;
  };
};

#endif  
