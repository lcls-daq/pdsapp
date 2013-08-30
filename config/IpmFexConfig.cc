#include "pdsapp/config/IpmFexConfig.hh"
#include "pdsapp/config/IpmFexTable.hh"

#include "pds/config/DiodeFexConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"

#include <new>

typedef DiodeFexConfigType T;
typedef IpmFexConfigType   U;

static const int NRANGES = 8;
static const int NCHAN = 4;

using namespace Pds_ConfigDb;

IpmFexConfig::IpmFexConfig():
  Serializer("Ipm_Fex"), _table(new IpmFexTable(NRANGES))
{
  _table->insert(pList);
}

int IpmFexConfig::readParameters(void *from)
{
  U& c = *reinterpret_cast<U*>(from);
  for(int i=0; i<NCHAN; i++)
    _table->set(i,
                c.diode()[i].base ().data(),
                c.diode()[i].scale().data());
  _table->xscale(c.xscale());
  _table->yscale(c.yscale());
  return sizeof(U);
}

int IpmFexConfig::writeParameters(void *to)
{
  T darray[NCHAN];
  for(int i=0; i<NCHAN; i++)
    _table->get(i,
                const_cast<float*>(darray[i].base ().data()),
                const_cast<float*>(darray[i].scale().data()));
  *new(to) U(darray,_table->xscale(),_table->yscale());
  return sizeof(U);
}

int IpmFexConfig::dataSize() const
{
  return sizeof(U);
}
