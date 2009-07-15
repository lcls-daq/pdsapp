#ifndef odfEpicsCA_hh
#define odfEpicsCA_hh

//--------------------------------------------------------------------------
//
// File and Version Information:
// 	$Id$
//
// Environment:
//      This software was developed for the BaBar collaboration.  If you
//      use all or part of it, please give an appropriate acknowledgement.
//      This class was copied from OdcCompProxy/EpicsCAProxy for use
//      in the online environment.
//
//
//------------------------------------------------------------------------

// epics includes
#include "cadef.h"

class EpicsCA;

//==============================================================================
class EpicsCAChannel {
public:
  enum ConnStatus { NotConnected, Connecting, Connected };

  EpicsCAChannel(const char* channelName,
		 EpicsCA&    proxy);
  ~EpicsCAChannel();
    
  void connect           (void);
  void dataCallback      (struct event_handler_args ehArgs);
  void connStatusCallback(struct connection_handler_args chArgs);
  const char* epicsName  (void) { return _epicsName; }
  int setVar             (const void* value);
  int monitor            ();
  ConnStatus connected   () { return _connected; }
  
protected:
  const char* _epicsName;
  chid	      _epicsChanID;
  chtype      _type;
  evid        _event;
  ConnStatus  _connected;
  EpicsCA&    _proxy;
};

//==============================================================================
class EpicsCA {
public:
  EpicsCA(const char *channelName);
  virtual ~EpicsCA();
  
  //  virtual void connected   (int isConnected, int id) = 0;
  virtual void dataCallback(const void* value) = 0;
protected:
  EpicsCAChannel _channel;
};

#endif
