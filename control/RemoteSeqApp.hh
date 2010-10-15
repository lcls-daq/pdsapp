#ifndef Pds_RemoteSeqApp_hh
#define Pds_RemoteSeqApp_hh

#include "pds/utility/Appliance.hh"
#include "pds/service/Routine.hh"

#include "pds/config/ControlConfigType.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {
  class PartitionControl;
  class PVManager;
  class StateSelect;
  class Src;
  class Task;
  
  class RemoteSeqApp : public Appliance,
		       public Routine {
  public:
    RemoteSeqApp(PartitionControl& control,
		 StateSelect&      manual,
		 PVManager&        pvmanager,
		 const Src&        src);
    ~RemoteSeqApp();
  public:
    virtual Transition* transitions(Transition*);
    virtual Occurrence* occurrences(Occurrence*);
    virtual InDatagram* events     (InDatagram*);
    virtual void        routine    ();
  private:
    bool readTransition();
  private:
    PartitionControl&  _control;
    StateSelect&       _manual;
    PVManager&         _pvmanager;
    Xtc                _configtc;
    char*              _config_buffer;
    char*              _cfgmon_buffer;
    bool               _done;
    Task*              _task;
    unsigned short     _port;
    volatile int       _socket;
    volatile bool      _wait_for_configure;
  };
};

#endif
