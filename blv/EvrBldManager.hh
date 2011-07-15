#ifndef Pds_EvrBldManager_hh
#define Pds_EvrBldManager_hh

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <memory.h>
#include <time.h>
#include <stdlib.h>

#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/config/EvrConfigType.hh" 
#include "pds/service/Task.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/client/Fsm.hh"

#include "pds/config/CfgClientNfs.hh"
#include "pdsapp/blv/EvrBldServer.hh"

namespace Pds {

  class Evr;
  class DetInfo;

class EvrBldManager {
public:
  EvrBldManager(const DetInfo& src,
                const char* evrid);
  ~EvrBldManager();
  void allocate (Transition*);
  void configure(Transition*);
  void start();
  void stop();
  void enable();
  void disable();
  void reset();
  void handleEvrIrq();
  EvrBldServer& server() { return _evrBldServer; }
  Appliance& appliance() { return _fsm; }


private:
  EvgrBoardInfo<Evr>  _erInfo;
  Evr&                _er;
  EvrBldServer        _evrBldServer;
  CfgClientNfs        _cfg;
  unsigned            _evtCounter;
  Fsm                 _fsm;
  char*               _configBuffer;
};

}

#endif


