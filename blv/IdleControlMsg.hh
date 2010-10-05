#ifndef Pds_IdleControl_hh
#define Pds_IdleControl_hh

namespace Pds {
  class IdleControlMsg {
  public:
    char dbpath[64];
    int  key;
  };
};

#endif

