#ifndef Pds_EvrBldManager_hh
#define Pds_EvrBldManager_hh

#include <list>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <memory.h>
#include <time.h>
#include <stdlib.h>

#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/service/Task.hh"
#include "EvrConfigType.hh" 
#include "EvrBldServer.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/config/IpimbDataType.hh"

namespace Pds {

  class Fsm;
  class Appliance;
  class Evr;

  class PulseParams {
  public:
    enum Polarity { Positive, Negative };
  public:
    unsigned eventcode;
    unsigned delay;
    unsigned width;
    unsigned output;
    enum Polarity polarity;
  };

  
class EvrBldManager {
public:

  EvrBldManager(EvgrBoardInfo<Evr>& erInfo, 
                const std::list<PulseParams>& pulses,
                EvrBldServer& evrBldServer,CfgClientNfs** cfgIpimb, 
                unsigned nIpimbServers,unsigned* bldIdMap);
  ~EvrBldManager() { }
  void configure();
  void start();
  void stop();
  void enable();
  void disable();
  void reset();
  void handleEvrIrq();
  EvrBldServer& server() { return _evrBldServer; }
  Appliance& appliance() { return _fsm; }


private:
  Evr&   _er;
  std::list<PulseParams> _pulses;
  EvrBldServer& _evrBldServer;
  EvgrBoardInfo<Evr>*   _erInfo;
  unsigned _evtCounter;
  Fsm& _fsm;
  IpimbConfigType* _ipimbConfig;
 
  
};

}

#endif


