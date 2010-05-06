#ifndef Pds_EvrEventCode_hh
#define Pds_EvrEventCode_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

namespace Pds_ConfigDb {
  class EvrEventCode {
  public:
    EvrEventCode();
  public:   
    void id(unsigned);
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull(void* from);
    int  push(void* to);
  private:
    NumericInt<unsigned>          _code;
    Enumerated<Enums::Bool>       _isReadout;
    Enumerated<Enums::Bool>       _isTerminator;
    NumericInt<unsigned>          _reportDelay;
    NumericInt<unsigned>          _reportWidth;    
    NumericInt<unsigned>          _maskTrigger;
    NumericInt<unsigned>          _maskSet;
    NumericInt<unsigned>          _maskClear;
  };
};

#endif    
