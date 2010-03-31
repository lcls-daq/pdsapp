#ifndef Pds_EvrPulseConfig_hh
#define Pds_EvrPulseConfig_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

namespace Pds_ConfigDb {
  class EvrPulseConfig {
  public:
    EvrPulseConfig();
  public:   
    void id(unsigned);
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull(void* from);
    int  push(void* to);
  private:
    unsigned                      _pulse;
    Enumerated<Enums::Polarity>   _polarity;
    NumericInt<unsigned>          _prescale;
    NumericInt<unsigned>          _delay;
    NumericInt<unsigned>          _width;
  };
};

#endif    
