#ifndef Pds_EvrOutputMap_hh
#define Pds_EvrOutputMap_hh

#include "pds/config/EvrConfigType.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

namespace Pds_ConfigDb {
  class EvrOutputMap {
  public:
    EvrOutputMap();
  public:   
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull  (const OutputMapType&);
    int push   (void* to);
  private:
    Enumerated<OutputMapType::Source> _source;
    NumericInt<unsigned>              _source_id;
    Enumerated<OutputMapType::Conn>   _conn;
    NumericInt<unsigned>              _conn_id;
    NumericInt<unsigned>              _module;
  };
};

#endif
