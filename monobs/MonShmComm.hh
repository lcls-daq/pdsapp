#ifndef Pds_MonShmComm_hh
#define Pds_MonShmComm_hh

#include <stdint.h>

namespace Pds {
  namespace MonShmComm {

    enum { ServerPort = 5719 };

    class Get {
    public:
      char     hostname[32];
      uint32_t groups;
      uint32_t mask;
      uint32_t events;
      uint32_t dmg;
    };

    class Set {
    public:
      Set() {}
      Set(unsigned m) : mask(m) {}
    public:
      uint32_t mask;
    };
  };
};

#endif
