#include "pdsapp/config/DiodeFexConfig.hh"
#include "pdsapp/config/DiodeFexTable.hh"

using namespace Pds_ConfigDb;

DiodeFexConfig::DiodeFexConfig() : 
  Serializer("DiodeFexConfig"),
  _table( new DiodeFexTable )
{
  _table->insert(pList);
}

int DiodeFexConfig::readParameters (void* from) {
  return _table->pull(from);
}

int  DiodeFexConfig::writeParameters(void* to) {
  return _table->push(to);
}

int  DiodeFexConfig::dataSize() const {
  return _table->dataSize();
}
