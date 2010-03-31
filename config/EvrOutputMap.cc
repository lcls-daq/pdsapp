#include "EvrOutputMap.hh"

#include "pds/config/EvrConfigType.hh"

using namespace Pds_ConfigDb;

static const char* source_range[] = { "Pulse",
              "DBus",
              "Prescaler",
              "Force_High",
              "Force_Low",
              NULL };
static const char* conn_range[] = { "Front Panel",
            "Univ IO",
            NULL };

EvrOutputMap::EvrOutputMap() :
  _source    ("Source"   , EvrConfigType::OutputMapType::Pulse, source_range),
  _source_id ("Source id", 0, 0, 9),
  _conn      ("Conn"     , EvrConfigType::OutputMapType::FrontPanel, conn_range),
  _conn_id   ("Conn id"  , 0, 0, 9)
{}
   
void EvrOutputMap::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_source);
  pList.insert(&_source_id);
  pList.insert(&_conn);
  pList.insert(&_conn_id);
}

bool EvrOutputMap::pull(void* from) {
  const EvrConfigType::OutputMapType& tc = * (const EvrConfigType::OutputMapType*) from;
  _source   .value = tc.source();
  _source_id.value = tc.source_id();
  _conn     .value = tc.conn();
  _conn_id  .value = tc.conn_id();
  return true;
}

int EvrOutputMap::push(void* to) {
  EvrConfigType::OutputMapType& tc = *new(to) EvrConfigType::OutputMapType(
    _source.value,
    _source_id.value,
    _conn.value,
    _conn_id.value);
  return sizeof(tc);
}

#include "Parameters.icc"

template class Enumerated<EvrConfigType::OutputMapType::Source>;
template class Enumerated<EvrConfigType::OutputMapType::Conn>;
