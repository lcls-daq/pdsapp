#ifndef PdsCas_PVMonitorCb_hh
#define PdsCas_PVMonitorCb_hh

namespace PdsCas {
  class PVMonitorCb {
  public:
    virtual ~PVMonitorCb() {}
    virtual void updated() = 0;
  };
};

#endif
