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
    theProxy->dataCallback(ehArgs);
  }
  
}

EpicsCA::EpicsCA(const char* channelName) :
  _channel(channelName,*this)
{ 
}

EpicsCA::~EpicsCA()
{
}


EpicsCAChannel::EpicsCAChannel(const char* channelName,
			       EpicsCA&    proxy) :
  _epicsName(channelName),
  _connected(NotConnected),
  _proxy    (proxy)
{
  const int priority = 0;
  int st = ca_create_channel(_epicsName, ConnStatusCB, this, priority, &_epicsChanID);
  if (st != ECA_NORMAL) 
    printf("%s : %s\n", _epicsName, ca_message(st));
}

EpicsCAChannel::~EpicsCAChannel()
{
  if (_connected != NotConnected)
    ca_clear_subscription(_event);

  ca_clear_channel(_epicsChanID);
}

void EpicsCAChannel::connStatusCallback(struct connection_handler_args chArgs)
{
  printf("%s: connStatusCallback\n",_epicsName);

  _connected = NotConnected;
  
  switch (ca_state(chArgs.chid)) {
  case cs_prev_conn:
    // valid chid, IOC was found, but unavailable
    printf("%s : disconnected\n", _epicsName);
    break;
  case cs_conn:          // valid chid, IOC was found, still available
    printf("%s : connected\n", _epicsName);
    _connected = Connected;
    monitor();
    break;
  default:
  case cs_never_conn:    // valid chid, IOC not found
  case cs_closed:        // invalid chid
    printf("%s : no such variable\n",_epicsName);
    break;
  }
  
}

void EpicsCAChannel::dataCallback(struct event_handler_args ehArgs)
{
  printf("%s: dataCallback (%d)\n",_epicsName,ehArgs.status);

  if (ehArgs.status != ECA_NORMAL)
    printf("%s : %s\n",_epicsName, ca_message(ehArgs.status));
  //  else if (ehArgs.type != DBR_STRING)
  else if (ehArgs.type != _type)
    printf("%s : unexpected data type returned\n",_epicsName);
  else
    _proxy.dataCallback(ehArgs.dbr);
} 

int EpicsCAChannel::setVar(const void* value)
{
  if ( _connected!=Connected ) {
    printf("%s : error setting state; channel not connected\n",_epicsName);
    return false;
  }
  
  int st = ca_put_callback(_type, _epicsChanID, value,
			   GetDataCallback, this);
  if (st != ECA_NORMAL) {
    _printf("%s : %s\n",_epicsName,ca_message(st));
    return 0;
  }
  
  return 1;
}

int EpicsCAChannel::monitor()
{
  // establish monitoring
  int st = ca_create_subscription(_type = ca_field_type(_epicsChanID),
				  ca_element_count(_epicsChanID),
				  _epicsChanID,
				  DBE_VALUE | DBE_ALARM,
				  // DBE_ALARM,
				  GetDataCallback,
				  this,
				  &_event);
  
  if (st != ECA_NORMAL) {
    printf("%s : %s\n", _epicsName, ca_message(st));
    return 0;
  }
  
  return 1;
}


