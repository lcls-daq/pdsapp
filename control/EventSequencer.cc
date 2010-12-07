#include "pdsapp/control/EventSequencer.hh"

#include "pds/config/EvrConfigType.hh"
#include "pds/config/SeqConfigType.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/evr/SequencerEntry.hh"

#include <stdio.h>

static const int MaxConfigSize = 0x100000;

using namespace Pds;

static char _buff[64];
const char* pvname(const char* format, const char* base, unsigned id)
{
  sprintf(_buff,format,base,id);
  return _buff;
}

static const char* ecs_ioc = "IOC:IN20:EV01";

EventSequencer::EventSequencer(unsigned id) :
  _config           (DetInfo(0,DetInfo::NoDetector,0,DetInfo::Evr,0)),
  _config_buffer    (new char[MaxConfigSize]),
  _event_code_writer(pvname("%s:ECS_SEQ_%d.A",ecs_ioc,id)),
  _beam_delay_writer(pvname("%s:ECS_SEQ_%d.B",ecs_ioc,id)),
  _fid_delay_writer (pvname("%s:ECS_SEQ_%d.C",ecs_ioc,id)),
  _seq_length_writer(pvname("%s:ECS_LEN_%d",ecs_ioc,id)),
  _seq_plymod_writer(pvname("%s:ECS_PLYMOD_%d",ecs_ioc,id)),
  _seq_repcnt_writer(pvname("%s:ECS_REPCNT_%d",ecs_ioc,id)),
  _seq_proc_writer  (pvname("%s:ECS_SEQ_%d.PROC",ecs_ioc,id)),
  _seq_cntl_writer  (pvname("%s:ECS_PLYCTL_%d",ecs_ioc,id)),
  _ca_context       (ca_current_context())
{
}

void   EventSequencer::initialize(const Allocation& alloc)
{
  _config.initialize(alloc);
}

Damage EventSequencer::configure(const Transition& tr) 
{
  Damage damage(0);
  int len = _config.fetch(tr, _evrConfigType, _config_buffer, MaxConfigSize);
  if (len <= 0) {
    printf("EventSequencer: failed to retrieve configuration.\n");
    damage.increase(Damage::UserDefined);
  }
  else {
    const SeqConfigType& seqConfig = 
      reinterpret_cast<const EvrConfigType*>(_config_buffer)->seq_config();

    if (seqConfig.sync_source() != SeqConfigType::Disable) {

      int* event_code_array = reinterpret_cast<int*>(_event_code_writer.data());
      int* beam_delay_array = reinterpret_cast<int*>(_beam_delay_writer.data());
      int* fid_delay_array  = reinterpret_cast<int*>(_fid_delay_writer .data());
      printf("EventSequencer len %d\n",seqConfig.length());

      for(unsigned i=0; i<seqConfig.length(); i++) {
	const Pds::EvrData::SequencerEntry& entry = seqConfig.entry(i);
	event_code_array[i] = entry.eventcode();
	beam_delay_array[i] = 0;
	fid_delay_array [i] = entry.delay();
        printf("EventSequencer step %d: %d/%d\n",
               i,entry.eventcode(),entry.delay());
      }

      *reinterpret_cast<int*>(_seq_length_writer.data()) = seqConfig.length();

      int* plymod = reinterpret_cast<int*>(_seq_plymod_writer.data());
      int* repcnt = reinterpret_cast<int*>(_seq_repcnt_writer.data());
      switch(seqConfig.cycles()) {
      case 0 : *plymod = 2; *repcnt = 0; break;
      case 1 : *plymod = 0; *repcnt = 0; break;
      default: *plymod = 1; *repcnt = seqConfig.cycles(); break;
      }

      *reinterpret_cast<int*>(_seq_proc_writer  .data()) = 1;

      _event_code_writer.put();
      _beam_delay_writer.put();
      _fid_delay_writer .put();
      _seq_length_writer.put();
      _seq_plymod_writer.put();
      _seq_repcnt_writer.put();
      _seq_proc_writer  .put();

      ca_flush_io();

      printf("EventSequencer put\n");
    }
  }
  return damage;
}

void EventSequencer::start()
{
  const SeqConfigType& seqConfig = 
    reinterpret_cast<const EvrConfigType*>(_config_buffer)->seq_config();
  if (seqConfig.sync_source() != SeqConfigType::Disable) {
    *reinterpret_cast<int*>(_seq_cntl_writer  .data()) = 1;
    _seq_cntl_writer  .put();
    ca_flush_io();

    printf("EventSequencer start\n");
  }
}

void EventSequencer::stop()
{
  if (_ca_context) {
    ca_attach_context(_ca_context);
    _ca_context = 0;
  }

  const SeqConfigType& seqConfig = 
    reinterpret_cast<const EvrConfigType*>(_config_buffer)->seq_config();
  if (seqConfig.sync_source() != SeqConfigType::Disable) {
    *reinterpret_cast<int*>(_seq_cntl_writer  .data()) = 0;
    _seq_cntl_writer  .put();
    ca_flush_io();

    printf("EventSequencer stop\n");
  }
}

/*
Validate against 
amo-daq:~> caget IOC:IN20:EV01:ECS_MINEC_1
IOC:IN20:EV01:ECS_MINEC_1      67 
amo-daq:~> caget IOC:IN20:EV01:ECS_MAXEC_1
IOC:IN20:EV01:ECS_MAXEC_1      74 
*/
