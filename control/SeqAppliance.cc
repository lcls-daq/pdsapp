#include "SeqAppliance.hh"
#include "pdsapp/control/EventSequencer.hh"
#include "pdsapp/control/PVManager.hh"
#include "pdsapp/control/ConfigSelect.hh"
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
			   ConfigSelect&     cselect,
			   CfgClientNfs&     config,
			   PVManager&        pvmanager,
                           unsigned          sequencer_id) :
  _control      (control),
  _manual       (manual),
  _cselect      (cselect),
  _config       (config),
  _configtc     (_controlConfigType, config.src()),
  _config_buffer(new char[MaxConfigSize]),
  _cur_config   (0),
  _end_config   (0),
  _pvmanager    (pvmanager),
  _sequencer_id (sequencer_id),
  _sequencer    (0),
  _ca_context   (ca_current_context())
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
    { 
      const Allocation& alloc = reinterpret_cast<const Allocate&>(*tr).allocation();
      _config.initialize(alloc);

      if (_ca_context) {
        ca_attach_context(_ca_context);
        _ca_context = 0;
      }
      if (_sequencer_id) {
        _sequencer = new EventSequencer(_sequencer_id);
        _sequencer->initialize(alloc);
        _control.set_sequencer(_sequencer);
      }
    } break;
  case TransitionId::Unmap:
    if (_sequencer)
      delete _sequencer;
    break;
  case TransitionId::Configure:
    {
      Damage damage(0);
      if (_sequencer)
        damage.increase(_sequencer->configure(*tr).value());

      //
      //  Enable control configuration
      //
      int len = _config.fetch(*tr, _controlConfigType, _config_buffer, MaxConfigSize);
      if (len <= 0) {
	printf("SeqAppliance: failed to retrieve configuration.  Applying default.\n");
	len = (ControlConfig::_new(_config_buffer))->_sizeof();
      }

      _cur_config = reinterpret_cast<ControlConfigType*>(_config_buffer);
      _end_config = _cur_config->_sizeof() >= unsigned(len) ? 
	0 : _config_buffer + len;
      if (len > 0) 
	printf("SeqAppliance configure tc %p (0x%x)  len 0x%x\n",
	       _cur_config, _cur_config->_sizeof(), len);

      _configtc.extent = sizeof(Xtc) + _cur_config->_sizeof();
      _configtc.damage = damage;
      _control.set_transition_payload(TransitionId::Configure,&_configtc,_cur_config);
      break;
    }
  case TransitionId::BeginCalibCycle:
    //  apply the configuration
    _control.set_transition_env(TransitionId::Enable, 
				_cur_config->uses_duration() ?
				EnableEnv(_cur_config->duration()).value() :
				EnableEnv(_cur_config->events()).value());
    if (_cselect.controlpvs())
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
    _cselect.configured_(true);
  case TransitionId::BeginCalibCycle:
    _done = false;
    //    dg->insert(_configtc,_cur_config); 
    break;
  case TransitionId::EndCalibCycle:
    //  advance the configuration
    { 
      int len = _cur_config->_sizeof();
      char* nxt_config = reinterpret_cast<char*>(_cur_config)+len;
      if (nxt_config < _end_config) {
	_cur_config = reinterpret_cast<ControlConfigType*>(nxt_config);
	printf("SeqAppliance endcalib tc %p  done %c\n",
	       _cur_config, _done ? 't':'f');
	if (_done) _control.set_target_state(PartitionControl::Enabled);
      }
      else {
	_cur_config = reinterpret_cast<ControlConfigType*>(_config_buffer);
	printf("SeqAppliance endcalib stop at tc %p  done %c\n",
	       _cur_config, _done ? 't':'f');
	if (_done) _control.set_target_state(PartitionControl::Configured);
      }
      _configtc.extent = sizeof(Xtc) + _cur_config->_sizeof();
    }
    break;
  case TransitionId::Unconfigure:
    _cselect.configured_(true);
    break;
  default:
    break;
  }
  return dg; 
}
