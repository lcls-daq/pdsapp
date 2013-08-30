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
  _source    ("Source"   , OutputMapType::Pulse, source_range),
  _source_id ("Source id", 0, 0, 255),
  _conn      ("Conn"     , OutputMapType::FrontPanel, conn_range),
  _conn_id   ("Conn id"  , 0, 0, 12),
  _module    ("Module"   , 0, 0, 255)
{}
   
void EvrOutputMap::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_source);
  pList.insert(&_source_id);
  pList.insert(&_conn);
  pList.insert(&_conn_id);
  pList.insert(&_module);
}

bool EvrOutputMap::pull(const OutputMapType& tc) {
  _source   .value = tc.source();
  _source_id.value = tc.source_id();
  _conn     .value = tc.conn();
  _conn_id  .value = tc.conn_id();
  _module   .value = tc.module ();
  return true;
}

int EvrOutputMap::push(void* to) {
  OutputMapType& tc = *new(to) OutputMapType(
    _source.value,
    _source_id.value,
    _conn.value,
    _conn_id.value,
    _module.value);
  return sizeof(tc);
}

#include "Parameters.icc"

template class Enumerated<OutputMapType::Source>;
template class Enumerated<OutputMapType::Conn>;
