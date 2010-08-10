#include "pdsapp/config/IpmFexConfig.hh"
#include "pdsapp/config/IpmFexTable.hh"

using namespace Pds_ConfigDb;

IpmFexConfig::IpmFexConfig():
  Serializer("Ipm_Fex"), _table(new IpmFexTable)
{
  _table->insert(pList);
}

int IpmFexConfig::readParameters(void *from)
{
  return _table->pull(from);
}

int IpmFexConfig::writeParameters(void *to)
{
  return _table->push(to);
}

int IpmFexConfig::dataSize() const
{
  return _table->dataSize();
}
