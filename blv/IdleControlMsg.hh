#ifndef Pds_IdleControl_hh
#define Pds_IdleControl_hh

#include <stdint.h>

namespace Pds {
  class IdleControlMsg {
  public:
    char     dbpath[64];
    int32_t  key;
  };
};

#endif

