#ifndef PdsConfigDb_PVControl_hh
#define PdsConfigDb_PVControl_hh

#include "pdsapp/config/Parameters.hh"

namespace Pds_ConfigDb {

  class PVControl {
  public:
    PVControl();
    
    void insert(Pds::LinkedList<Parameter>& pList);

    bool pull(void* from);
    int  push(void* to);

  private:
    TextParameter        _name;
    NumericFloat<double> _value;
  };
};

#endif
