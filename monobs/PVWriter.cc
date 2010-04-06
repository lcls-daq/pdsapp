#include "PVWriter.hh"

#include "db_access.h"

#include <stdio.h>

#define handle_type(ctype, stype, dtype) case ctype:	\
  { struct stype* ival = (struct stype*)dbr;		\
    dtype* inp  = &ival->value;				\
    dtype* outp = (dtype*)_pvdata;			\
    for(int k=0; k<nelem; k++) *outp++ = *inp++;	\
  }

using namespace PdsCas;

PVWriter::PVWriter(const char* pvName) :
  EpicsCA(pvName,false),
  _connected(false), _pvdata(0)
{
}

PVWriter::~PVWriter() 
{
  if (_pvdata) 
    delete[] _pvdata; 
}

void PVWriter::connected   (bool c) { _connected = c; }

void PVWriter::getData     (const void* dbr)  
{
  if (_pvdata) 
    delete _pvdata;

  int nelem = _channel.nelements();
  int sz = dbr_size_n(_channel.type(),nelem);
  _pvdata = new char[sz];
  printf("pvdata allocated @ %p sz %d type %d\n",_pvdata,sz,_channel.type());

  switch(_channel.type()) {
    handle_type(DBR_TIME_SHORT , dbr_time_short , dbr_short_t ) break;
    handle_type(DBR_TIME_FLOAT , dbr_time_float , dbr_float_t ) break;
    handle_type(DBR_TIME_ENUM  , dbr_time_enum  , dbr_enum_t  ) break;
    handle_type(DBR_TIME_LONG  , dbr_time_long  , dbr_long_t  ) break;
    handle_type(DBR_TIME_DOUBLE, dbr_time_double, dbr_double_t) break;
  default: printf("Unknown type %d\n", int(_channel.type())); break;
  }
      
}

void  PVWriter::put         () { _channel.put(); }

void* PVWriter::putData     () 
{
  return _pvdata; 
}

void  PVWriter::putStatus   (bool s) {}
