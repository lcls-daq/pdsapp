#ifndef Pds_EventcodeTiming_hh
#define Pds_EventcodeTiming_hh

namespace Pds_ConfigDb {
  class EventcodeTiming {
  public:
    static void     timeslot(unsigned* ticks);
    static unsigned timeslot(unsigned code);
    static unsigned period  (unsigned code);
  };
}

#endif
