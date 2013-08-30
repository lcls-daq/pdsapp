#include "EvrOutputMap_V1.hh"

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

EvrOutputMap_V1::EvrOutputMap_V1() :
  _source    ("Source"   , Pds::EvrData::OutputMap::Pulse, source_range),
  _source_id ("Source id", 0, 0, 255),
  _conn      ("Conn"     , Pds::EvrData::OutputMap::FrontPanel, conn_range),
  _conn_id   ("Conn id"  , 0, 0, 9)
{}
   
void EvrOutputMap_V1::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_source);
  pList.insert(&_source_id);
  pList.insert(&_conn);
  pList.insert(&_conn_id);
}

bool EvrOutputMap_V1::pull(const Pds::EvrData::OutputMap& tc) {
  _source   .value = tc.source();
  _source_id.value = tc.source_id();
  _conn     .value = tc.conn();
  _conn_id  .value = tc.conn_id();
  return true;
}

int EvrOutputMap_V1::push(void* to) {
  Pds::EvrData::OutputMap& tc = *new(to) Pds::EvrData::OutputMap(
    _source.value,
    _source_id.value,
    _conn.value,
    _conn_id.value);
  return sizeof(tc);
}

#include "Parameters.icc"

template class Enumerated<Pds::EvrData::OutputMap::Source>;
template class Enumerated<Pds::EvrData::OutputMap::Conn>;
