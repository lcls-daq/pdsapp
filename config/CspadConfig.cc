#include "pdsapp/config/CspadConfig.hh"
#include "pdsapp/config/CspadConfigTable.hh"

using namespace Pds_ConfigDb;

CspadConfig::CspadConfig():
  Serializer("Cspad_Config"), _table(new CspadConfigTable(*this))
{
  _table->insert(pList);
}

int CspadConfig::readParameters(void *from)
{
  return _table->pull(from);
}

int CspadConfig::writeParameters(void *to)
{
  return _table->push(to);
}

int CspadConfig::dataSize() const
{
  return _table->dataSize();
}

bool CspadConfig::validate() 
{
  return _table->validate();
}
