#include "pdsapp/config/Cspad2x2Config.hh"
#include "pdsapp/config/Cspad2x2ConfigTable.hh"

using namespace Pds_ConfigDb;

Cspad2x2Config::Cspad2x2Config():
  Serializer("Cspad2x2_Config"), _table(new Cspad2x2ConfigTable(*this))
{
  _table->insert(pList);
  this->name("Cspad 140K Configuration");
}

int Cspad2x2Config::readParameters(void *from)
{
  return _table->pull(*reinterpret_cast<const CsPad2x2ConfigType*>(from));
}

int Cspad2x2Config::writeParameters(void *to)
{
  return _table->push(to);
}

int Cspad2x2Config::dataSize() const
{
  return _table->dataSize();
}

bool Cspad2x2Config::validate()
{
  return _table->validate();
}
