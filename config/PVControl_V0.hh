#ifndef PdsConfigDb_PVControl_hh
#define PdsConfigDb_PVControl_hh

#include "pdsapp/config/Parameters.hh"

#include "pdsdata/psddl/control.ddl.h"

namespace Pds_ConfigDb {
  namespace PVControl_V0 {
    class PVControl {
    public:
      PVControl();
    
      void insert(Pds::LinkedList<Parameter>& pList);
      bool pull  (const Pds::ControlData::PVControl& tc);
      int  push  (void* to);

    private:
      TextParameter        _name;
      NumericFloat<double> _value;
    };
  };
};

#endif
