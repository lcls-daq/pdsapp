#include "SeqAppliance.hh"
#include "pdsapp/control/PVManager.hh"
#include "pdsapp/control/StateSelect.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pdsdata/xtc/TransitionId.hh"

#include "cadef.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const int MaxConfigSize = 0x100000;

using namespace Pds;

SeqAppliance::SeqAppliance(PartitionControl& control,
			   StateSelect&      manual,
			   CfgClientNfs&     config,
			   PVManager&        pvmanager ) :
  _control      (control),
  _manual       (manual),
  _config       (config),
  _configtc     (_controlConfigType, config.src()),
  _config_buffer(new char[MaxConfigSize]),
  _cur_config   (0),
  _end_config   (0),
  _pvmanager    (pvmanager)
{
}

SeqAppliance::~SeqAppliance()
{
  delete[] _config_buffer;
}

Transition* SeqAppliance::transitions(Transition* tr) 
{ 
  if (!_manual.control_enabled())
    return tr;

  switch(tr->id()) {
  case TransitionId::Map:
    //  EPICS thread initialization
    SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), 
	     "PVDisplay calling ca_context_create" );
    _config.initialize(reinterpret_cast<const Allocate&>(*tr).allocation());
    break;
  case TransitionId::Unmap:
    //  EPICS thread cleanup
    ca_context_destroy();
    break;
  case TransitionId::Configure:
    {
      int len = _config.fetch(*tr, _controlConfigType, _config_buffer, MaxConfigSize);
      if (len <= 0) {
	printf("SeqAppliance: failed to retrieve configuration.  Applying default.\n");
	len = (new(_config_buffer) ControlConfigType(Pds::ControlData::ConfigV1::Default))->size();
      }

      _cur_config = reinterpret_cast<ControlConfigType*>(_config_buffer);
      _end_config = _cur_config->size() >= unsigned(len) ? 
	0 : _config_buffer + len;
      _configtc.extent = sizeof(Xtc) + _cur_config->size();
      _control.set_transition_payload(TransitionId::Configure,&_configtc,_cur_config);
    }
  case TransitionId::BeginCalibCycle:
    //  apply the configuration
    _control.set_transition_env(TransitionId::Enable, 
				_cur_config->uses_duration() ?
				EnableEnv(_cur_config->duration()).value() :
				EnableEnv(_cur_config->events()).value());
    _pvmanager.configure(*_cur_config);
    _control.set_transition_payload(TransitionId::BeginCalibCycle,&_configtc,_cur_config);
    break;
  case TransitionId::EndCalibCycle:
  case TransitionId::Unconfigure:
    _pvmanager.unconfigure();
  default:
    break;
  }
  return tr;
}

Occurrence* SeqAppliance::occurrences(Occurrence* occ) 
{
  if (!_manual.control_enabled())
    return occ;

  if (occ->id() == OccurrenceId::SequencerDone) {
    _done = true;
    _control.set_target_state(PartitionControl::Running);
    return 0;
  }

  return occ; 
}

InDatagram* SeqAppliance::events     (InDatagram* dg) 
{ 
  if (!_manual.control_enabled())
    return dg;

  switch(dg->datagram().seq.service()) {
  case TransitionId::Configure:
  case TransitionId::BeginCalibCycle:
    _done = false;
    //    dg->insert(_configtc,_cur_config); 
    break;
  case TransitionId::EndCalibCycle:
    //  advance the configuration
    { 
      int len = _cur_config->size();
      char* nxt_config = reinterpret_cast<char*>(_cur_config)+len;
      if (nxt_config < _end_config) {
	_cur_config = reinterpret_cast<ControlConfigType*>(nxt_config);
	if (_done) _control.set_target_state(PartitionControl::Enabled);
      }
      else {
	_cur_config = reinterpret_cast<ControlConfigType*>(_config_buffer);
	if (_done) _control.set_target_state(PartitionControl::Unmapped);
      }
      _configtc.extent = sizeof(Xtc) + _cur_config->size();
    }
    break;
  default:
    break;
  }
  return dg; 
}
