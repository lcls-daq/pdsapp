#include "RemoteSeqApp.hh"
#include "StateSelect.hh"
#include "ConfigSelect.hh"
#include "PartitionSelect.hh"
#include "PVManager.hh"
#include "RemotePartition.hh"
#include "pdsapp/tools/SummaryDg.hh"

#include "pds/management/PartitionControl.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pds/service/Task.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"
#include "pdsdata/psddl/control.ddl.h"

#include "pds/config/ControlConfigType.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>

#define DBUG

static const int MaxConfigSize = 0x100000;
static const int Control_Port  = 10130;

using namespace Pds;

RemoteSeqApp::RemoteSeqApp(PartitionControl& control,
         StateSelect&      manual,
         ConfigSelect&     cselect,
         PVManager&        pvmanager,
         const Src&        src,
         RunStatus&        runStatus,
         PartitionSelect&  pselect) :
  _control      (control),
  _manual       (manual),
  _select       (cselect),
  _pvmanager    (pvmanager),
  _runStatus    (runStatus),
  _pselect      (pselect),
  _configtc     (_controlConfigType, src),
  _config_buffer(new char[MaxConfigSize]),
  _cfgmon_buffer(new char[MaxConfigSize]),
  _task         (new Task(TaskObject("remseq"))),
  _port         (Control_Port + control.header().platform()),
  _last_run     (0,0),
  _socket       (-1),
  _wait_for_configure(false),
  _l3t_events   (0)
{
  _task->call(this);
}

RemoteSeqApp::~RemoteSeqApp()
{
  _task->destroy();
  delete[] _config_buffer;
  delete[] _cfgmon_buffer;
}

bool RemoteSeqApp::readTransition()
{
  RemoteSeqCmd&       cmd =
    *reinterpret_cast<RemoteSeqCmd*>(_config_buffer);
  int len = ::recv(_socket, &cmd, sizeof(cmd), MSG_WAITALL);

  if (len == (int) sizeof(RemoteSeqCmd))
  {
    if (cmd.IsValid())
      return processTransitionCmd(cmd);
  }

  ControlConfigType&  config =
    *reinterpret_cast<ControlConfigType*>(_config_buffer);
  int len2 = ::recv(_socket, (char*) (&cmd + 1), sizeof(config) - sizeof(cmd), MSG_WAITALL);
  len += len2;

  if (len != sizeof(config)) {
    if (errno==0)
      printf("RemoteSeqApp: remote end closed\n");
    else
      printf("RemoteSeqApp failed to read config hdr(%d/%zu) : %s\n",
       len,sizeof(config),strerror(errno));
    return false;
  }
  int payload = config._sizeof()-sizeof(config);
  if (payload>0) {
    len = ::recv(_socket, &config+1, payload, MSG_WAITALL);
    if (len != payload) {
      printf("RemoteSeqApp failed to read config payload(%d/%d) : %s\n",
       len,payload,strerror(errno));
      return false;
    }
  }
  if (config.uses_duration())
    printf("received remote configuration for %d/%d seconds, %d controls\n",
           config.duration().seconds(),
           config.duration().nanoseconds(),
           config.npvControls());
  else if (config.uses_events())
    printf("received remote configuration for %d events, %d controls\n",
           config.events(),
           config.npvControls());

  //
  //  Create config with only DAQ control information (to send to EVR)
  //
  std::list<ControlData::PVControl> controls;
  std::list<ControlData::PVMonitor> monitors;
  std::list<ControlData::PVLabel  > labels;

  if (config.uses_duration())
    ControlConfig::_new(_cfgmon_buffer, controls, monitors, labels, config.duration());
  else
    ControlConfig::_new(_cfgmon_buffer, controls, monitors, labels, config.events());

  _configtc.extent = sizeof(Xtc) + config._sizeof();
  return true;
}

bool RemoteSeqApp::processTransitionCmd(RemoteSeqCmd& cmd)
{
  switch (cmd.u32Type)
  {
  case RemoteSeqCmd::CMD_GET_CUR_EVENT_NUM:
    int64_t iEventNum = _runStatus.getEventNum();
    ::write(_socket,&iEventNum,sizeof(iEventNum));
#ifdef DBUG
    printf("RemoteSeqApp get events [%ld]\n",long(iEventNum));
#endif
    break;
  }

  return true; // keep DAQ running
}

void RemoteSeqApp::routine()
{
  ControlConfigType& config =
    *reinterpret_cast<ControlConfigType*>(_config_buffer);

  //  replace the configuration with default running
  ControlConfig::_new(_config_buffer);
  _configtc.extent = sizeof(Xtc) + config._sizeof();

  int listener;
  if ((listener = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("RemoteSeqApp::routine failed to allocate socket : %s\n",
     strerror(errno));
  }
  else {
    int optval=1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
      printf("RemoteSeqApp::routine failed to setsockopt : %s\n",
       strerror(errno));
      close(listener);
    }
    else {
      Ins src(_port);
      Sockaddr sa(src);
      if (::bind(listener, sa.name(), sa.sizeofName()) < 0) {
  printf("RemoteSeqApp::routine failed to bind to %x/%d : %s\n",
         src.address(),src.portId(),strerror(errno));
        close(listener);
      }
      else {
        while(::listen(listener, 5) >= 0) {
          Sockaddr name;
          uint32_t length = name.sizeofName();
          int s = accept(listener, name.name(), &length);
          if (_control.current_state()==PartitionControl::Unmapped) {
            printf("RemoteSeqApp rejected connection while unmapped\n");
            close(s);
          }
          else {
            printf("RemoteSeqApp accepted connection from %x/%d\n",
                   name.get().address(), name.get().portId());

            //  Cache the recording state
            bool lrecord = _manual.record_state();

            _manual.disable_control();
            _select.enable_control(false);
            
            _socket = s;

            //  First, send the current configdb and run key
            length = strlen(_control.partition().dbpath());
            ::write(_socket,&length,sizeof(length));
            ::write(_socket,_control.partition().dbpath(),length);

            uint32_t old_key = _control.get_transition_env(TransitionId::Configure);
            ::write(_socket,&old_key,sizeof(old_key));

            // send the current config type (alias)
            string sType = _select.getType();
            length = sType.size();
            ::write(_socket,&length,sizeof(length));
            ::write(_socket,sType.c_str(),length);

	    //
	    // cache then send the partition selection
	    //
	    const Allocation* palloc = new Allocation(_control.partition());

	    { RemotePartition* partition = new RemotePartition;
	      const QList<DetInfo>& detectors = _pselect.detectors();
	      const QList<ProcInfo>& segments = _pselect.segments ();
	      for(int j=0; j<detectors.size(); j++) {
		RemoteNode node(DetInfo::name(detectors[j]),detectors[j].phy());
		for(unsigned k=0; k<palloc->nnodes(); k++)
		  if (palloc->node(k)->procInfo() == segments[j]) {
		    node.record(!palloc->node(k)->transient());
		    break;
		  }
		partition->add_node(node);
	      }
	      length = sizeof(*partition);
	      ::write(_socket,&length,sizeof(length));
	      ::write(_socket,partition,length); 
	      delete partition;
	    }

            uint32_t options;
	    while(1) {

	      //  Receive the requested run key and recording option
	      int len = ::recv(_socket, &options, sizeof(options), MSG_WAITALL);
	      if (len != sizeof(options)) {
		if (errno==0)
		  printf("RemoteSeqApp: remote end closed\n");
		else
		  printf("RemoteSeqApp failed to read options(%d/%zu) : %s\n",
			 len,sizeof(options),strerror(errno));
		break;
	      }

#ifdef DBUG
	      printf("Received options %x\n",options);
#endif

	      if (options & ModifyPartition) {
		RemotePartition* partition = new RemotePartition;
		int len = ::recv(_socket, partition, sizeof(*partition), MSG_WAITALL);
		if (len != sizeof(*partition)) {
		  if (errno==0)
		    printf("RemoteSeqApp: remote end closed\n");
		  else
		    printf("RemoteSeqApp failed to read partition(%d/%zu) : %s\n",
			   len,sizeof(*partition),strerror(errno));
		  break;
		}

#ifdef DBUG
		printf("Received partition %p\n",partition);
		for(unsigned j=0; j<partition->nodes(); j++) {
		  const RemoteNode& node = *partition->node(j);
		  printf("\t%s : %x : %s : %s\n",
			 node.name(),
			 node.phy(),
			 node.readout() ? "Readout":"NoReadout",
			 node.record () ? "Record":"NoRecord");
		}
#endif


		//
		//  Re-allocate
		//
		const QList<DetInfo>& detectors = _pselect.detectors();
		const QList<ProcInfo>& segments = _pselect.segments ();
		Allocation* alloc = new Allocation(_control.partition());
		for(unsigned j=0; j<partition->nodes(); j++) {
		  RemoteNode& node = *partition->node(j);
		  for(int k=0; k<detectors.size(); k++) {
		    if (detectors[k].phy() == node.phy()) {
		      if (!node.readout())
			alloc->remove(segments[k]);
		      else
			alloc->node(segments[k])->setTransient(!node.record());
		    }
		  }
		}
		_control.set_partition(*alloc);
		_control.set_target_state(PartitionControl::Unmapped);
		delete alloc;
	      }
	      _control.wait_for_target();
	      _control.set_target_state(PartitionControl::Mapped);
	      _control.release_target();
	      //
	      //  Reconfigure with the initial settings
	      //
	      if (!readTransition())  break;

	      _control.wait_for_target();
	      _control.set_transition_env    (TransitionId::Configure,options & DbKeyMask);
	      _control.set_transition_payload(TransitionId::Configure,&_configtc,_config_buffer);
	      
	      _control.set_target_state(PartitionControl::Configured);
	      
	      _wait_for_configure = true;
	      //	      _control.reconfigure(false);
	      _control.release_target();
	      
	      if (options & RecordSetMask)
		_manual.set_record_state( options & RecordValMask );
	      
	      while(1) {
		if (!readTransition())  break;
		//
		//  A request for a cycle of zero duration is an EndCalib
		//
		const Pds::ClockTime NoTime(0,0);
		if (config.uses_duration() && config.duration()==NoTime) {
		  _control.set_target_state(PartitionControl::Running);
		}
		else {
		  _control.wait_for_target();
		  _control.set_transition_payload(TransitionId::BeginCalibCycle,&_configtc,_config_buffer);
		  _control.set_transition_env(TransitionId::Enable,
					      config.uses_duration() ?
					      EnableEnv(config.duration()).value() :
					      EnableEnv(config.events()).value());
		  _control.set_target_state(PartitionControl::Enabled);
		  _control.release_target();
		}
	      }
	      break;
	    }
            close(_socket);
            _socket = -1;

	    //
	    //  Reallocate with the initial settings
	    //
            if (options & ModifyPartition) {
              _control.set_partition(*palloc);
              _control.set_target_state(PartitionControl::Unmapped);
              _control.wait_for_target();

              _control.set_target_state(PartitionControl::Mapped);
              _control.release_target();
            }

            delete palloc;

	    _control.wait_for_target();
            //  replace the configuration with default running
            ControlConfig::_new(_config_buffer);
            _configtc.extent = sizeof(Xtc) + config._sizeof();

            _control.set_transition_env    (TransitionId::Configure,old_key);
            _control.set_transition_payload(TransitionId::Configure,&_configtc,_config_buffer);
            _control.set_transition_env    (TransitionId::Enable,
                                            config.uses_duration() ?
                                            EnableEnv(config.duration()).value() :
                                            EnableEnv(config.events()).value());
	    _control.set_target_state(PartitionControl::Configured);
	    _control.release_target();

            _select.enable_control(true);
            _manual.enable_control();
            _manual.set_record_state(lrecord);
          }
        }
  printf("RemoteSeqApp::routine listen failed : %s\n",
         strerror(errno));
  close(listener);
      }
    }
  }
}

Transition* RemoteSeqApp::transitions(Transition* tr)
{
  if (_socket >= 0) {
    if (tr->id()==TransitionId::BeginRun) {
      if (tr->size() == sizeof(Transition))
        _last_run = RunInfo(0,0);
      else
        _last_run = *reinterpret_cast<RunInfo*>(tr);
    }
    else if (tr->id()==TransitionId::BeginCalibCycle) {
      _pvmanager.configure(*reinterpret_cast<ControlConfigType*>(_cfgmon_buffer));
      _l3t_events = 0;
    }
    else if (tr->id()==TransitionId::EndCalibCycle)
      _pvmanager.unconfigure();
  }

  return tr;
}

Occurrence* RemoteSeqApp::occurrences(Occurrence* occ)
{
  if (_socket >= 0) {
    int info = _last_run.run() | (_last_run.experiment()<<16);
    switch (occ->id()) {
    case OccurrenceId::SequencerDone:
      _control.set_target_state(PartitionControl::Running);
      return 0;
    case OccurrenceId::EvrEnabled:
      ::write(_socket,&info,sizeof(info));
#ifdef DBUG
      printf("RemoteSeqApp EvrEnabled [%d]\n",info);
#endif
      return 0;
    default:
      break;
    }
  }
  return occ;
}

InDatagram* RemoteSeqApp::events     (InDatagram* dg)
{
  if (_socket >= 0) {
    int id   = dg->datagram().seq.service();
    if (id==TransitionId::L1Accept &&
        dg->datagram().xtc.contains.value()==SummaryDg::typeId().value()) {
      const SummaryDg& s = *static_cast<const SummaryDg*>(dg);
      ControlConfigType& config =
        *reinterpret_cast<ControlConfigType*>(_config_buffer);
      if (config.uses_l3t_events() &&
          s.l3tresult()==SummaryDg::Pass &&
          ++_l3t_events == config.events()) {
        _control.set_target_state(PartitionControl::Running);
      }
      return 0;
    }
    int info = _last_run.run() | (_last_run.experiment()<<16);
    if (dg->datagram().xtc.damage.value()!=0) {
      info = -id;
      ::write(_socket,&info,sizeof(info));
#ifdef DBUG
      printf("RemoteSeqApp transition [%d] damaged [%x]\n",
             -info,dg->datagram().xtc.damage.value());
#endif
    }
    else if (_wait_for_configure) {
      if (id==TransitionId::Configure) {
	::write(_socket,&info,sizeof(info));
	_wait_for_configure = false;
#ifdef DBUG
        printf("RemoteSeqApp configure complete [%d]\n",info);
#endif
      }
    }
    else if (id==TransitionId::EndCalibCycle) {
      ::write(_socket,&info,sizeof(info));
#ifdef DBUG
        printf("RemoteSeqApp endcalib complete [%d]\n",info);
#endif
    }
  }
  return dg;
}
