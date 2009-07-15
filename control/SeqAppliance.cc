#include "SeqAppliance.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pdsdata/xtc/TransitionId.hh"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const int MaxConfigSize = 0x100000;

using namespace Pds;

SeqAppliance::SeqAppliance(PartitionControl& control,
			   CfgClientNfs&     config) :
  _control      (control),
  _config       (config),
  _configtc     (_controlConfigType, config.src()),
  _config_buffer(new char[MaxConfigSize]),
  _cur_config   (0),
  _end_config   (0)
{
}

SeqAppliance::~SeqAppliance()
{
  delete[] _config_buffer;
}

Transition* SeqAppliance::transitions(Transition* tr) 
{ 
  switch(tr->id()) {
  case TransitionId::Map:
    {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _config.initialize(alloc.allocation());
    }
    break;
  case TransitionId::Configure:
    {
      int len = _config.fetch(*tr, _controlConfigType, _config_buffer);
      if (len <= 0) {
	printf("SeqAppliance: failed to retrieve configuration.  Applying default.\n");
	len = (new(_config_buffer) ControlConfigType(Pds::ControlData::ConfigV1::Default))->size();
      }

      printf("SeqAppliance: configuration is size 0x%x bytes\n",len);
      _cur_config = reinterpret_cast<ControlConfigType*>(_config_buffer);
      _end_config = _cur_config->size() >= unsigned(len) ? 
	0 : _config_buffer + len;
      _configtc.extent = sizeof(Xtc) + _cur_config->size();
    }
  case TransitionId::BeginCalibCycle:
    //  apply the configuration
    _control.set_transition_env(TransitionId::Enable, 
				_cur_config->uses_duration() ?
				EnableEnv(_cur_config->duration()).value() :
				EnableEnv(_cur_config->events()).value());
    break;
  default:
    break;
  }
  return tr;
}

Occurrence* SeqAppliance::occurrences(Occurrence* occ) 
{
  if (occ->id() == OccurrenceId::SequencerDone) {
    _done = true;
    _control.set_target_state(PartitionControl::Running);
    return 0;
  }
  return occ; 
}

InDatagram* SeqAppliance::events     (InDatagram* dg) 
{ 
  switch(dg->datagram().seq.service()) {
  case TransitionId::Configure:
  case TransitionId::BeginCalibCycle:
    _done = false;
    dg->insert(_configtc,_cur_config);
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
