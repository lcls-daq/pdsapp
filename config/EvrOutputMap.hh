#ifndef Pds_EvrOutputMap_hh
#define Pds_EvrOutputMap_hh

#include "pdsdata/evr/OutputMap.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

namespace Pds_ConfigDb {
  class EvrOutputMap {
  public:
    EvrOutputMap();
  public:   
    void insert(Pds::LinkedList<Parameter>& pList);
    bool pull(void* from);
    int push(void* to);
  private:
    Enumerated<Pds::EvrData::OutputMap::Source> _source;
    NumericInt<unsigned>               _source_id;
    Enumerated<Pds::EvrData::OutputMap::Conn> _conn;
    NumericInt<unsigned>             _conn_id;
  };
};

#endif
