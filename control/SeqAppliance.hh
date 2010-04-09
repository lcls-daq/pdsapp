#ifndef Pds_SeqAppliance_hh
#define Pds_SeqAppliance_hh

#include "pds/utility/Appliance.hh"

#include "pds/config/ControlConfigType.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {
  class PartitionControl;
  class StateSelect;
  class CfgClientNfs;
  class PVManager;
  
  class SeqAppliance : public Appliance {
  public:
    SeqAppliance(PartitionControl& control,
		 StateSelect&      manual,
		 CfgClientNfs&     config,
		 PVManager&        pvmanager);
    ~SeqAppliance();
  public:
    virtual Transition* transitions(Transition*);
    virtual Occurrence* occurrences(Occurrence*);
    virtual InDatagram* events     (InDatagram*);
  private:
    PartitionControl&  _control;
    StateSelect&       _manual;
    CfgClientNfs&      _config;
    Xtc                _configtc;
    char*              _config_buffer;
    ControlConfigType* _cur_config;
    char*              _end_config;
    bool               _done;
    PVManager&         _pvmanager;
  };
};

#endif
