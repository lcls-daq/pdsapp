#include "pdsapp/config/EvrConfigP.hh"
#include "pdsapp/config/EvrPulseTable.hh"

using namespace Pds_ConfigDb;

EvrConfigP::EvrConfigP():
  Serializer("Evr_Config"), _table(new EvrPulseTable(*this))
{
  _table->insert(pList);
}

int EvrConfigP::readParameters(void *from)
{
  return _table->pull(from);
}

int EvrConfigP::writeParameters(void *to)
{
  return _table->push(to);
}

int EvrConfigP::dataSize() const
{
  return _table->dataSize();
}

bool EvrConfigP::validate() 
{
  return _table->validate();
}
