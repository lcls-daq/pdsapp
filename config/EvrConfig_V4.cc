#include "pdsapp/config/EvrConfig_V4.hh"
#include "pdsapp/config/EvrPulseTable_V4.hh"

using namespace Pds_ConfigDb::EvrConfig_V4;

EvrConfig::EvrConfig():
  Serializer("Evr_Config"), _table(new EvrPulseTable_V4(*this))
{
  _table->insert(pList);
}

int EvrConfig::readParameters(void *from)
{
  return _table->pull(*reinterpret_cast<const EvrConfigType*>(from));
}

int EvrConfig::writeParameters(void *to)
{
  return _table->push(to);
}

int EvrConfig::dataSize() const
{
  return _table->dataSize();
}

bool EvrConfig::validate() 
{
  return _table->validate();
}
