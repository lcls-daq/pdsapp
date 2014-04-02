#ifndef Pds_SeqAppliance_hh
#define Pds_SeqAppliance_hh

#include "pds/utility/Appliance.hh"

#include "pds/config/ControlConfigType.hh"
#include "pdsdata/xtc/Xtc.hh"

struct ca_client_context;

namespace Pds {
  class PartitionControl;
  class StateSelect;
  class ConfigSelect;
  class CfgClientNfs;
  class PVManager;
  class EventSequencer;
  
  class SeqAppliance : public Appliance {
  public:
    SeqAppliance(PartitionControl& control,
		 CfgClientNfs&     config,
		 PVManager&        pvmanager,
                 unsigned          sequencer_id);
    SeqAppliance(PartitionControl& control,
		 StateSelect&      manual,
		 ConfigSelect&     cselect,
		 CfgClientNfs&     config,
		 PVManager&        pvmanager,
                 unsigned          sequencer_id);
    ~SeqAppliance();
  public:
    virtual Transition* transitions(Transition*);
    virtual Occurrence* occurrences(Occurrence*);
    virtual InDatagram* events     (InDatagram*);
  private:
    PartitionControl&  _control;
    StateSelect&       _manual;
    ConfigSelect&      _cselect;
    CfgClientNfs&      _config;
    Xtc                _configtc;
    Xtc                _containertc;
    char*              _config_buffer;
    ControlConfigType* _cur_config;
    char*              _end_config;
    bool               _done;
    PVManager&         _pvmanager;
    unsigned           _sequencer_id;
    EventSequencer*    _sequencer;
    struct ca_client_context* _ca_context;
  };
};

#endif
