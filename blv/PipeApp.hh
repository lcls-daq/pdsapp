#ifndef Pds_PipeApp_hh
#define Pds_PipeApp_hh

#include "pds/utility/Appliance.hh"

namespace Pds {
  class PipeApp : public Appliance {
  public:
    PipeApp(int read_fd, int write_fd);
  public:
    InDatagram* events     (InDatagram*);
    Transition* transitions(Transition*);
  private:
    int _read_fd;
    int _write_fd;
  };
}

#endif
