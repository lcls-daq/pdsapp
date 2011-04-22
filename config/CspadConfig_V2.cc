#include "pdsapp/config/CspadConfig_V2.hh"
#include "pdsapp/config/CspadConfigTable_V2.hh"

using namespace Pds_ConfigDb;

CspadConfig_V2::CspadConfig_V2():
  Serializer("Evr_Config"), _table(new CspadConfigTable_V2(*this))
{
  _table->insert(pList);
}

int CspadConfig_V2::readParameters(void *from)
{
  return _table->pull(from);
}

int CspadConfig_V2::writeParameters(void *to)
{
  return _table->push(to);
}

int CspadConfig_V2::dataSize() const
{
  return _table->dataSize();
}

bool CspadConfig_V2::validate()
{
  return _table->validate();
}
