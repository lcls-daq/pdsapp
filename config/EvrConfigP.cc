#include "pdsapp/config/EvrConfigP.hh"
#include "pdsapp/config/EvrPulseTable.hh"

using namespace Pds_ConfigDb;

EvrConfig::EvrConfig():
  Serializer("Evr_Config"), _table(new EvrPulseTable(*this))
{
  _table->insert(pList);
}

int EvrConfig::readParameters(void *from)
{
  return _table->pull(from);
}

int EvrConfig::writeParameters(void *to)
{
  return _table->push(to);
}

int EvrConfig::dataSize() const
{
  return _table->dataSize();
}
