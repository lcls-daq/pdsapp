#include "pdsapp/config/EvrConfig_V4.hh"
#include "pdsapp/config/EvrPulseTable_V4.hh"

using namespace Pds_ConfigDb;

EvrConfig_V4::EvrConfig_V4():
  Serializer("Evr_Config"), _table(new EvrPulseTable_V4(*this))
{
  _table->insert(pList);
}

int EvrConfig_V4::readParameters(void *from)
{
  return _table->pull(from);
}

int EvrConfig_V4::writeParameters(void *to)
{
  return _table->push(to);
}

int EvrConfig_V4::dataSize() const
{
  return _table->dataSize();
}

bool EvrConfig_V4::validate() 
{
  return _table->validate();
}
