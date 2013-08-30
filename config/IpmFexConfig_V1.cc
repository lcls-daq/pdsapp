#include "pdsapp/config/IpmFexConfig_V1.hh"
#include "pdsapp/config/IpmFexTable.hh"

#include "pdsdata/psddl/lusi.ddl.h"

#include <new>

typedef Pds::Lusi::DiodeFexConfigV1 T;
typedef Pds::Lusi::IpmFexConfigV1   U;

static const int NCHAN = 4;

using namespace Pds_ConfigDb;

IpmFexConfig_V1::IpmFexConfig_V1():
  Serializer("Ipm_Fex"), _table(new IpmFexTable(T::NRANGES))
{
  _table->insert(pList);
}

int IpmFexConfig_V1::readParameters(void *from)
{
  U& c = *reinterpret_cast<U*>(from);
  for(int i=0; i<NCHAN; i++)
    _table->set(i,c.diode()[i].base ().data(),c.diode()[i].scale().data());
  _table->xscale(c.xscale());
  _table->yscale(c.yscale());
  return sizeof(U);
}

int IpmFexConfig_V1::writeParameters(void *to)
{
  T darray[NCHAN];
  for(int i=0; i<NCHAN; i++)
    _table->get(i,
                const_cast<float*>(darray[i].base ().data()),
                const_cast<float*>(darray[i].scale().data()));
  *new(to) U(darray,_table->xscale(),_table->yscale());
  return sizeof(U);
}

int IpmFexConfig_V1::dataSize() const
{
  return sizeof(U);
}
