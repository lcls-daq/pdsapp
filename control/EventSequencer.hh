#ifndef Pds_EventSequencer_hh
#define Pds_EventSequencer_hh

#include "pds/management/Sequencer.hh"

#include "pds/epicstools/PVWriter.hh"
#include "pds/config/CfgClientNfs.hh"

#include "pdsdata/xtc/Damage.hh"

namespace Pds {
  class Allocation;
  class Transition;
  class EventSequencer : public Sequencer {
  public:
    EventSequencer(unsigned id);
  public:
    void   initialize(const Allocation&);
    Damage configure (const Transition&);
    void   start();
    void   stop ();
  private:
    CfgClientNfs _config;
    char*        _config_buffer;
    Pds_Epics::PVWriter     _event_code_writer;
    Pds_Epics::PVWriter     _beam_delay_writer;
    Pds_Epics::PVWriter     _fid_delay_writer ;
    Pds_Epics::PVWriter     _seq_length_writer;
    Pds_Epics::PVWriter     _seq_plymod_writer;
    Pds_Epics::PVWriter     _seq_repcnt_writer;
    Pds_Epics::PVWriter     _seq_proc_writer  ;
    Pds_Epics::PVWriter     _seq_cntl_writer  ;
    struct ca_client_context* _ca_context;
  };
};

#endif
