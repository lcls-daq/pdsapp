#ifndef Pds_EvrEventCodeV3_hh
#define Pds_EvrEventCodeV3_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

#include "pdsdata/psddl/evr.ddl.h"

namespace Pds_ConfigDb {
  class EvrEventCodeV3 {
  public:
    EvrEventCodeV3();
  public:   
    void id    (unsigned);
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull  (const Pds::EvrData::EventCodeV3&);
    int  push  (void* to);
  private:
    NumericInt<unsigned>          _code;
    Enumerated<Enums::Bool>       _isReadout;
    Enumerated<Enums::Bool>       _isTerminator;
    NumericInt<unsigned>          _maskTrigger;
    NumericInt<unsigned>          _maskSet;
    NumericInt<unsigned>          _maskClear;
  };
};

#endif    
