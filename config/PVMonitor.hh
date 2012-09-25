#ifndef PdsConfigDb_PVMonitor_hh
#define PdsConfigDb_PVMonitor_hh

#include "pdsapp/config/Parameters.hh"

namespace Pds_ConfigDb {

  class PVMonitor {
  public:
    PVMonitor();
    
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull(void* from);
    int  push(void* to);
  private:
    TextParameter        _name;
    NumericFloat<double> _loValue;
    NumericFloat<double> _hiValue;
  };
};

#endif
