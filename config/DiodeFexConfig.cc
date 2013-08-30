#include "pdsapp/config/DiodeFexConfig.hh"
#include "pdsapp/config/DiodeFexTable.hh"

#include "pds/config/DiodeFexConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

typedef DiodeFexConfigType T;

// limit to 8 ranges
static const int NRANGES=8;

DiodeFexConfig::DiodeFexConfig() : 
  Serializer("DiodeFexConfig"),
  _table( new DiodeFexTable(NRANGES) ) 
{
  _table->insert(pList);
}

int DiodeFexConfig::readParameters (void* from) {
  T& c = *new(from) T;
  _table->pull(const_cast<float*>(c.base ().data()),
               const_cast<float*>(c.scale().data()));
  return sizeof(c);
}

int  DiodeFexConfig::writeParameters(void* to) {
  float b[T::NRANGES];
  float s[T::NRANGES];
  _table->push(b,s);
  *new(to) T(b,s);
  return sizeof(T);
}

int  DiodeFexConfig::dataSize() const {
  return sizeof(T);
}
