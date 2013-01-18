#include "pdsapp/config/Cspad2x2Config_V1.hh"
#include "pdsapp/config/Cspad2x2ConfigTable_V1.hh"

using namespace Pds_ConfigDb;

Cspad2x2Config_V1::Cspad2x2Config_V1():
  Serializer("Cspad2x2_Config_V1"), _table(new Cspad2x2ConfigTable_V1(*this))
{
  _table->insert(pList);
  this->name("Cspad 140K Configuration V1");
}

int Cspad2x2Config_V1::readParameters(void *from)
{
  return _table->pull(from);
}

int Cspad2x2Config_V1::writeParameters(void *to)
{
  return _table->push(to);
}

int Cspad2x2Config_V1::dataSize() const
{
  return _table->dataSize();
}

bool Cspad2x2Config_V1::validate()
{
  return _table->validate();
}
