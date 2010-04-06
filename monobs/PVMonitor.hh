#ifndef Pds_PVMonitor_hh
#define Pds_PVMonitor_hh

#include "EpicsCA.hh"

namespace PdsCas {
  class PVMonitor : public EpicsCA {
  public:
    PVMonitor(const char* pvName, PVMonitorCb& cb) : EpicsCA(pvName,&cb) {}
    ~PVMonitor() {}
  };

};

#endif
