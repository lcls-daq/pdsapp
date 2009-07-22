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
    EpicsCAChannel* theProxy = (EpicsCAChannel*) ca_puser(chArgs.chid);
    theProxy->connStatusCallback(chArgs);
  }
  
  extern "C" void GetDataCallback(struct event_handler_args ehArgs)
  {
    EpicsCAChannel* theProxy = (EpicsCAChannel*) ehArgs.usr;
    theProxy->getDataCallback(ehArgs);
  }
  
  extern "C" void PutDataCallback(struct event_handler_args ehArgs)
  {
    EpicsCAChannel* theProxy = (EpicsCAChannel*) ehArgs.usr;
    theProxy->putDataCallback(ehArgs);
  }
  
}

EpicsCA::EpicsCA(const char* channelName,
		 bool monitor) :
  _channel(channelName,monitor,*this)
{ 
}

EpicsCA::~EpicsCA()
{
}


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
    printf("%s : %s\n", _epicsName, ca_message(st));
}

EpicsCAChannel::~EpicsCAChannel()
{
  if (_connected != NotConnected && _monitor)
    ca_clear_subscription(_event);

  ca_clear_channel(_epicsChanID);
}

void EpicsCAChannel::connStatusCallback(struct connection_handler_args chArgs)
{
  if ( chArgs.op == CA_OP_CONN_UP ) {
    _connected = Connected;
    int dbfType = ca_field_type(_epicsChanID);

    _proxy.connected(true);

    int dbrType = dbf_type_to_DBR_TIME(dbfType);
    if (dbr_type_is_ENUM(dbrType))
      dbrType = DBR_TIME_INT;
    
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
    _connected = NotConnected;
    _proxy.connected(false);
  }
}

void EpicsCAChannel::getDataCallback(struct event_handler_args ehArgs)
{
  if (ehArgs.status != ECA_NORMAL)
    printf("%s : %s [getDataCallback ehArgs]\n",_epicsName, ca_message(ehArgs.status));
  else {
    _proxy.getData(ehArgs.dbr);

    int dbfType = ca_field_type(_epicsChanID);

    if (!_monitor) {
      int st = ca_array_put_callback (dbfType,
				      _nelements,
				      _epicsChanID,
				      _proxy.putData(),
				      PutDataCallback,
				      this);
      if (st != ECA_NORMAL)
	printf("%s : %s [getDataCallback st]\n",_epicsName, ca_message(st));

      ca_flush_io();
    }
  }
} 

void EpicsCAChannel::putDataCallback(struct event_handler_args ehArgs)
{
  if (ehArgs.status != ECA_NORMAL)
    printf("%s : %s\n",_epicsName, ca_message(ehArgs.status));
  _proxy.putStatus(ehArgs.status==ECA_NORMAL);
}

