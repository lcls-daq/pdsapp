#ifndef Pds_EvrBldManager_hh
#define Pds_EvrBldManager_hh

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <memory.h>
#include <time.h>
#include <stdlib.h>
#include <list>

#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/service/Task.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/client/Fsm.hh"

#include "pds/evgr/EvrCfgClient.hh"
#include "pds/mon/THist.hh"

namespace Pds {

  class Evr;
  class DetInfo;

class EvrBldManager {
public:
  EvrBldManager(const DetInfo&        src,
                const char*           evrid,
                const std::list<int>& write_fd);
  ~EvrBldManager();
  void allocate (Transition*);
  void configure(Transition*);
  void start();
  void stop();
  void enable();
  void disable();
  void reset();
  void handleEvrIrq();
  Appliance& appliance() { return _fsm; }


private:
  EvgrBoardInfo<Evr>  _erInfo;
  Evr&                _er;
  std::list<int>      _write_fd;
  EvrCfgClient        _cfg;
  unsigned            _evtCounter;
  Fsm                 _fsm;
  char*               _configBuffer;

  timespec            _tsignal;
  THist               _hsignal;
};

}

#endif


