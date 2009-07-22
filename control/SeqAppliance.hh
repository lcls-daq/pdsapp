#ifndef Pds_SeqAppliance_hh
#define Pds_SeqAppliance_hh

#include "pds/utility/Appliance.hh"

#include "pds/config/ControlConfigType.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {
  class PartitionControl;
  class CfgClientNfs;
  class PVManager;
  
  class SeqAppliance : public Appliance {
  public:
    SeqAppliance(PartitionControl& control,
		 CfgClientNfs&     config,
		 PVManager&        pvmanager);
    ~SeqAppliance();
  public:
    virtual Transition* transitions(Transition*);
    virtual Occurrence* occurrences(Occurrence*);
    virtual InDatagram* events     (InDatagram*);
  private:
    PartitionControl&  _control;
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
