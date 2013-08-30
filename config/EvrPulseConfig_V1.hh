#ifndef Pds_EvrPulseConfig_V1_hh
#define Pds_EvrPulseConfig_V1_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

#include "pdsdata/psddl/evr.ddl.h"

namespace Pds_ConfigDb {
  class EvrPulseConfig_V1 {
  public:
    EvrPulseConfig_V1();
  public:   
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull  (const Pds::EvrData::PulseConfig& tc);
    int push   (void* to);
  private:
    NumericInt<unsigned>    _pulse;
    NumericInt<int>         _trigger;
    NumericInt<int>         _set;
    NumericInt<int>         _clear;
    Enumerated<Enums::Polarity>    _polarity;
    Enumerated<Enums::Enabled>     _map_set_ena;
    Enumerated<Enums::Enabled>     _map_rst_ena;
    Enumerated<Enums::Enabled>     _map_trg_ena;
    NumericInt<unsigned>    _prescale;
    NumericInt<unsigned>    _delay;
    NumericInt<unsigned>    _width;
  };
};

#endif    
