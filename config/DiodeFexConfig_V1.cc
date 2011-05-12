#include "pdsapp/config/DiodeFexConfig_V1.hh"
#include "pdsapp/config/DiodeFexTable.hh"

#include "pdsdata/lusi/DiodeFexConfigV1.hh"

#include <new>

using namespace Pds_ConfigDb;

typedef Pds::Lusi::DiodeFexConfigV1 T;

DiodeFexConfig_V1::DiodeFexConfig_V1() : 
  Serializer("DiodeFexConfig_V1"),
  _table( new DiodeFexTable(T::NRANGES) )
{
  _table->insert(pList);
}

int DiodeFexConfig_V1::readParameters (void* from) {
  T& c = *new(from) T;
  _table->pull(c.base,c.scale);
  return sizeof(c);
}

int  DiodeFexConfig_V1::writeParameters(void* to) {
  float b[T::NRANGES];
  float s[T::NRANGES];
  _table->push(b,s);
  *new(to) T(b,s);
  return sizeof(T);
}

int  DiodeFexConfig_V1::dataSize() const {
  return sizeof(T);
}
