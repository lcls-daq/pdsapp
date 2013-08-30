#include "pdsapp/config/CspadConfig_V1.hh"
#include "pdsapp/config/CspadConfigTable_V1.hh"

using namespace Pds_ConfigDb;

CspadConfig_V1::CspadConfig_V1():
  Serializer("Evr_Config"), _table(new CspadConfigTable_V1(*this))
{
  _table->insert(pList);
}

int CspadConfig_V1::readParameters(void *from)
{
  return _table->pull(*reinterpret_cast<const Pds::CsPad::ConfigV1*>(from));
}

int CspadConfig_V1::writeParameters(void *to)
{
  return _table->push(to);
}

int CspadConfig_V1::dataSize() const
{
  return _table->dataSize();
}

bool CspadConfig_V1::validate() 
{
  return _table->validate();
}
