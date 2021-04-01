#ifndef PdsConfigDb_PVMonitor_hh
#define PdsConfigDb_PVMonitor_hh

#include "pdsapp/config/Parameters.hh"

#include "pds/config/ControlConfigType.hh"

namespace Pds_ConfigDb {

  class PVMonitor {
  public:
    PVMonitor();
    
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull  (const PVMonitorType& tc);
    int  push  (void* to);
  private:
    TextParameter        _name;
    NumericFloat<double> _loValue;
    NumericFloat<double> _hiValue;
  };
};

#endif
