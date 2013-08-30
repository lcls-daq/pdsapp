#ifndef PdsConfigDb_PVMonitor_hh
#define PdsConfigDb_PVMonitor_hh

#include "pdsapp/config/Parameters.hh"

#include "pdsdata/psddl/control.ddl.h"

namespace Pds_ConfigDb {

  class PVMonitor {
  public:
    PVMonitor();
    
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull  (const Pds::ControlData::PVMonitor& tc);
    int  push  (void* to);
  private:
    TextParameter        _name;
    NumericFloat<double> _loValue;
    NumericFloat<double> _hiValue;
  };
};

#endif
