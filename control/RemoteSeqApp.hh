#ifndef Pds_RemoteSeqApp_hh
#define Pds_RemoteSeqApp_hh

#include "RunStatus.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/Routine.hh"

#include "pds/config/ControlConfigType.hh"
#include "pdsdata/xtc/Xtc.hh"

#include "pdsapp/control/RemoteSeqCmd.hh"

namespace Pds {
  class PartitionControl;
  class PVManager;
  class ConfigSelect;
  class StateSelect;
  class Src;
  class Task;

  class RemoteSeqApp : public Appliance,
           public Routine {
  public:
    RemoteSeqApp(PartitionControl& control,
     StateSelect&      manual,
     ConfigSelect&     cselect,
     PVManager&        pvmanager,
     const Src&        src,
     RunStatus&        run,
     PartitionSelect&   pselect);
    ~RemoteSeqApp();
  public:
    virtual Transition* transitions(Transition*);
    virtual Occurrence* occurrences(Occurrence*);
    virtual InDatagram* events     (InDatagram*);
    virtual void        routine    ();
  private:
    bool readTransition();
    bool processTransitionCmd(RemoteSeqCmd& cmd);
  private:
    PartitionControl&  _control;
    StateSelect&       _manual;
    ConfigSelect&      _select;
    PVManager&         _pvmanager;
    RunStatus&         _runStatus;
    PartitionSelect&   _pselect;
    Xtc                _configtc;
    char*              _config_buffer;
    char*              _cfgmon_buffer;
    bool               _done;
    Task*              _task;
    unsigned short     _port;
    RunInfo            _last_run;
    volatile int       _socket;
    volatile bool      _wait_for_configure;
    unsigned           _l3t_events;
  };
};

#endif
