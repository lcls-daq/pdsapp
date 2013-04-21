#include "pdsapp/config/CspadConfig_V4.hh"
#include "pdsapp/config/CspadConfigTable_V4.hh"

using namespace Pds_ConfigDb;

CspadConfig_V4::CspadConfig_V4():
  Serializer("Cspad_Config"), _table(new CspadConfigTable_V4(*this))
{
  _table->insert(pList);
}

int CspadConfig_V4::readParameters(void *from)
{
  return _table->pull(from);
}

int CspadConfig_V4::writeParameters(void *to)
{
  return _table->push(to);
}

int CspadConfig_V4::dataSize() const
{
  return _table->dataSize();
}

bool CspadConfig_V4::validate()
{
  return _table->validate();
}
