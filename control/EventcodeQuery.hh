#ifndef Pds_EventcodeQuery_hh
#define Pds_EventcodeQuery_hh

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  class EventcodeQuery : public Pds_Epics::EpicsCA {
  public:
    static void execute();
  private:
    EventcodeQuery();
    ~EventcodeQuery();
  public:
    void connected(bool);
    void getData(const void*);
  };
};

#endif
