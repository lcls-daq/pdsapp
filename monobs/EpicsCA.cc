//--------------------------------------------------------------------------
//
// File and Version Information:
// 	$Id$
//
// Environment:
//      This software was developed for the BaBar collaboration.  If you
//      use all or part of it, please give an appropriate acknowledgement.
//
//
//------------------------------------------------------------------------

#include "EpicsCA.hh"

#include <string.h>
#include <stdlib.h>

// epics includes
#include "db_access.h"

#include <stdio.h>

namespace {

  extern "C" void ConnStatusCB(struct connection_handler_args chArgs)
  {
    PdsCas::EpicsCAChannel* theProxy = (PdsCas::EpicsCAChannel*) ca_puser(chArgs.chid);
    theProxy->connStatusCallback(chArgs);
  }
  
  extern "C" void GetDataCallback(struct event_handler_args ehArgs)
  {
    PdsCas::EpicsCAChannel* theProxy = (PdsCas::EpicsCAChannel*) ehArgs.usr;
    theProxy->getDataCallback(ehArgs);
  }
  
  extern "C" void PutDataCallback(struct event_handler_args ehArgs)
  {
    PdsCas::EpicsCAChannel* theProxy = (PdsCas::EpicsCAChannel*) ehArgs.usr;
    theProxy->putDataCallback(ehArgs);
  }
  
}

#define handle_type(ctype, stype, dtype) case ctype:	\
  { struct stype* ival = (struct stype*)dbr;		\
    dtype* inp  = &ival->value;				\
    dtype* outp = (dtype*)_pvdata;			\
    for(int k=0; k<nelem; k++) *outp++ = *inp++;	\
  }

using namespace PdsCas;

EpicsCA::EpicsCA(const char*   channelName,
		 PVMonitorCb*  monitor) :
  _channel(channelName,monitor!=0,*this),
  _monitor(monitor),
  _pvdata(new char[1]), 
  _pvsiz(0),
  _connected(false) 
{ 
}

EpicsCA::~EpicsCA()
{
  delete[] _pvdata; 
}

void EpicsCA::connected   (bool c) { _connected = c; }

bool EpicsCA::connected() const { return _connected; }

void EpicsCA::getData     (const void* dbr)  
{
  int nelem = _channel.nelements();
  int sz = dbr_size_n(_channel.type(),nelem);
  if (_pvsiz < sz) {
    delete[] _pvdata;
    _pvdata = new char[_pvsiz=sz];
    printf("pvdata allocated @ %p sz %d type %d\n",_pvdata,sz,int(_channel.type()));
  }

  switch(_channel.type()) {
    handle_type(DBR_TIME_SHORT , dbr_time_short , dbr_short_t ) break;
    handle_type(DBR_TIME_FLOAT , dbr_time_float , dbr_float_t ) break;
    handle_type(DBR_TIME_ENUM  , dbr_time_enum  , dbr_enum_t  ) break;
    handle_type(DBR_TIME_LONG  , dbr_time_long  , dbr_long_t  ) break;
    handle_type(DBR_TIME_DOUBLE, dbr_time_double, dbr_double_t) break;
  default: printf("Unknown type %d\n", int(_channel.type())); break;
  }

  if (_monitor) _monitor->updated();
}

void* EpicsCA::data        () 
{
  return _pvdata; 
}

size_t EpicsCA::data_size  () const { return _pvsiz; }

void  EpicsCA::putStatus   (bool s) {}


EpicsCAChannel::EpicsCAChannel(const char* channelName,
			       bool        monitor,
			       EpicsCA&    proxy) :
  _connected(NotConnected),
  _monitor  (monitor),
  _proxy    (proxy)
{
  snprintf(_epicsName, 32, channelName);
  strtok(_epicsName, "[");
//   char* index = strtok(NULL,"]");
//   if (index)
//     sscanf(index,"%d",&_element);
//   else
//     _element=0;

  const int priority = 0;
  int st = ca_create_channel(_epicsName, ConnStatusCB, this, priority, &_epicsChanID);
  if (st != ECA_NORMAL) 
    printf("EpicsCAChannel::ctor %s : %s\n", _epicsName, ca_message(st));
  else
    printf("EpicsCAChannel::ctor %s : chanID %d\n", _epicsName, _epicsChanID);
}

EpicsCAChannel::~EpicsCAChannel()
{
  if (_connected != NotConnected && _monitor)
    ca_clear_subscription(_event);

  ca_clear_channel(_epicsChanID);
}

void EpicsCAChannel::put()
{
  int dbfType = ca_field_type(_epicsChanID);
#if 0
  int st = ca_array_put_callback (dbfType,
				  _nelements,
				  _epicsChanID,
				  _proxy.data(),
				  PutDataCallback,
				  this);
#else
  int st = ca_array_put (dbfType, 
			 _nelements,
			 _epicsChanID,
			 _proxy.data());
#endif				  
  if (st != ECA_NORMAL)
    printf("%s : %s [put st] : %d\n",_epicsName, ca_message(st), dbfType);

  ca_flush_io();
}

void EpicsCAChannel::connStatusCallback(struct connection_handler_args chArgs)
{


  if ( chArgs.op == CA_OP_CONN_UP ) {
    printf("EpicsCAChannel::connStatusCallback %s connected (%p)\n",_epicsName, this);
    _connected = Connected;
    int dbfType = ca_field_type(_epicsChanID);

    _proxy.connected(true);

    int dbrType = dbf_type_to_DBR_TIME(dbfType);
    if (dbr_type_is_ENUM(dbrType))
      dbrType = DBR_TIME_INT;
    
    printf("EpicsCAChannel field type %d/%d\n",dbfType,dbrType);

    _type = dbrType;
    _nelements = ca_element_count(_epicsChanID);

    int st;
    if (_monitor) {
      // establish monitoring
      st = ca_create_subscription(_type,
				  _nelements,
				  _epicsChanID,
				  DBE_VALUE,
				  GetDataCallback,
				  this,
				  &_event);
    }
    else {
      st = ca_array_get_callback (_type,
				  _nelements,
				  _epicsChanID,
				  GetDataCallback,
				  this);
    }
    if (st != ECA_NORMAL)
      printf("%s : %s [connStatusCallback]\n", _epicsName, ca_message(st));
  }
  else {
    printf("EpicsCAChannel::connStatusCallback %s disconnected (%p)\n",_epicsName, this);
    _connected = NotConnected;
    _proxy.connected(false);
  }
}

void EpicsCAChannel::getDataCallback(struct event_handler_args ehArgs)
{
  printf("EpicsCAChannel::getDataCallback %s (%p)\n",_epicsName, this);

  if (ehArgs.status != ECA_NORMAL)
    printf("%s : %s [getDataCallback ehArgs]\n",_epicsName, ca_message(ehArgs.status));
  else {
    _proxy.getData(ehArgs.dbr);
  }
} 

void EpicsCAChannel::putDataCallback(struct event_handler_args ehArgs)
{
  if (ehArgs.status != ECA_NORMAL)
    printf("EpicsCAChannel::putDataCallback %s : %s\n",_epicsName, ca_message(ehArgs.status));
  _proxy.putStatus(ehArgs.status==ECA_NORMAL);
}

