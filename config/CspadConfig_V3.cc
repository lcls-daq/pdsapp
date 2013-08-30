#include "pdsapp/config/CspadConfig_V3.hh"
#include "pdsapp/config/CspadConfigTable_V3.hh"

using namespace Pds_ConfigDb;

CspadConfig_V3::CspadConfig_V3():
  Serializer("Evr_Config"), _table(new CspadConfigTable_V3(*this))
{
  _table->insert(pList);
}

int CspadConfig_V3::readParameters(void *from)
{
  return _table->pull(*reinterpret_cast<const Pds::CsPad::ConfigV3*>(from));
}

int CspadConfig_V3::writeParameters(void *to)
{
  return _table->push(to);
}

int CspadConfig_V3::dataSize() const
{
  return _table->dataSize();
}

bool CspadConfig_V3::validate()
{
  return _table->validate();
}
