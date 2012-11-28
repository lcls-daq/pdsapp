#include "RemoteSeqApp.hh"
#include "StateSelect.hh"
#include "ConfigSelect.hh"
#include "PVManager.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pds/service/Task.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"
#include "pdsdata/control/PVControl.hh"
#include "pdsdata/control/PVMonitor.hh"
#include "pdsdata/control/PVLabel.hh"

#include "pds/config/ControlConfigType.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const int MaxConfigSize = 0x100000;
static const int Control_Port  = 10130;

static const unsigned RecordSetMask = 0x20000;
static const unsigned RecordValMask = 0x10000;
static const unsigned DbKeyMask     = 0x0ffff;

using namespace Pds;

RemoteSeqApp::RemoteSeqApp(PartitionControl& control,
         StateSelect&      manual,
                           ConfigSelect&     cselect,
         PVManager&        pvmanager,
         const Src&        src,
         RunStatus&        runStatus) :
  _control      (control),
  _manual       (manual),
  _select       (cselect),
  _pvmanager    (pvmanager),
  _runStatus    (runStatus),
  _configtc     (_controlConfigType, src),
  _config_buffer(new char[MaxConfigSize]),
  _cfgmon_buffer(new char[MaxConfigSize]),
  _task         (new Task(TaskObject("remseq"))),
  _port         (Control_Port + control.header().platform()),
  _last_run     (0,0),
  _socket       (-1)
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
  ControlConfigType& config = 
    *reinterpret_cast<ControlConfigType*>(_config_buffer);

  //int len = ::recv(_socket, &config, sizeof(config), MSG_WAITALL);
  
  RemoteSeqCmd cmd;
  int len = ::recv(_socket, &cmd, sizeof(cmd), MSG_WAITALL);
  
  if (len == (int) sizeof(RemoteSeqCmd))
  {    
    if (cmd.IsValid())
      return processTransitionCmd(cmd);
  }
  
  memcpy((char*) &config, (char*) &cmd, sizeof(cmd));
  char* pConfigRemain = (char*) &config + sizeof(cmd);
  int len2 = ::recv(_socket, pConfigRemain, sizeof(config) - sizeof(cmd), MSG_WAITALL);  
  len += len2;
  
  if (len != sizeof(config)) {
    if (errno==0)
      printf("RemoteSeqApp: remote end closed\n");
    else
      printf("RemoteSeqApp failed to read config hdr(%d/%d) : %s\n",
       len,sizeof(config),strerror(errno));
    return false;
  }
  int payload = config.size()-sizeof(config);
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
    new(_cfgmon_buffer) ControlConfigType(controls, monitors, labels, config.duration());
  else
    new(_cfgmon_buffer) ControlConfigType(controls, monitors, labels, config.events  ());

  _configtc.extent = sizeof(Xtc) + config.size();
  return true;
}

bool RemoteSeqApp::processTransitionCmd(RemoteSeqCmd& cmd)
{  
  switch (cmd.u32Type)
  {
  case RemoteSeqCmd::CMD_GET_CUR_EVENT_NUM:
    int64_t iEventNum = _runStatus.getEventNum();
    ::write(_socket,&iEventNum,sizeof(iEventNum));
    break;
  }
  
  return true; // keep DAQ running
}

void RemoteSeqApp::routine()
{
  ControlConfigType& config = 
    *reinterpret_cast<ControlConfigType*>(_config_buffer);

  //  replace the configuration with default running
  new(_config_buffer) ControlConfigType(ControlConfigType::Default);
  _configtc.extent = sizeof(Xtc) + config.size();
  _control.set_transition_payload(TransitionId::Configure,&_configtc,_config_buffer);
  _control.set_transition_env(TransitionId::Enable, 
            config.uses_duration() ?
            EnableEnv(config.duration()).value() :
            EnableEnv(config.events()).value());

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
    unsigned length = name.sizeofName();
    _socket = accept(listener, name.name(), &length);
          if (_control.current_state()==PartitionControl::Unmapped) {
            printf("RemoteSeqApp rejected connection while unmapped\n");
            close(_socket);
            _socket = -1;
          }
          else {
            printf("RemoteSeqApp accepted connection from %x/%d\n",
                   name.get().address(), name.get().portId());

            //  Cache the recording state
            bool lrecord = _manual.record_state();

            _manual.disable_control();
            _select.enable_control(false);

            //  First, send the current configdb and run key
            length = strlen(_control.partition().dbpath());
            ::write(_socket,&length,sizeof(length));
            ::write(_socket,_control.partition().dbpath(),length);

            unsigned old_key = _control.get_transition_env(TransitionId::Configure);
            ::write(_socket,&old_key,sizeof(old_key));

            //  Receive the requested run key and recording option
            uint32_t options;
            int len = ::recv(_socket, &options, sizeof(options), MSG_WAITALL);
            if (len != sizeof(options)) {
              if (errno==0)
                printf("RemoteSeqApp: remote end closed\n");
              else
                printf("RemoteSeqApp failed to read options(%d/%d) : %s\n",
                       len,sizeof(options),strerror(errno));
            }

            //  Reconfigure with the initial settings
            else if (readTransition()) {
              _control.set_transition_env    (TransitionId::Configure,options & DbKeyMask);
              _control.set_transition_payload(TransitionId::Configure,&_configtc,_config_buffer);

              _control.set_target_state(PartitionControl::Configured);

              _wait_for_configure = true;
              _control.reconfigure();

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
                  _control.set_transition_payload(TransitionId::BeginCalibCycle,&_configtc,_config_buffer);
                  _control.set_transition_env(TransitionId::Enable, 
                                              config.uses_duration() ?
                                              EnableEnv(config.duration()).value() :
                                              EnableEnv(config.events()).value());
                  _control.set_target_state(PartitionControl::Enabled);
                }
              }
            }

            close(_socket);
            _socket = -1;

            //  replace the configuration with default running
            new(_config_buffer) ControlConfigType(ControlConfigType::Default);
            _configtc.extent = sizeof(Xtc) + config.size();

            _control.set_transition_env    (TransitionId::Configure,old_key);
            _control.set_transition_payload(TransitionId::Configure,&_configtc,_config_buffer);
            _control.set_transition_env    (TransitionId::Enable, 
                                            config.uses_duration() ?
                                            EnableEnv(config.duration()).value() :
                                            EnableEnv(config.events()).value());
            _control.set_target_state(PartitionControl::Configured);
            //            _control.reconfigure();

            _manual.enable_control();
            _select.enable_control(true);
    
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
    else if (tr->id()==TransitionId::BeginCalibCycle)
      _pvmanager.configure(*reinterpret_cast<ControlConfigType*>(_cfgmon_buffer));
    else if (tr->id()==TransitionId::EndCalibCycle)
      _pvmanager.unconfigure();
  }

  return tr;
}

Occurrence* RemoteSeqApp::occurrences(Occurrence* occ) 
{
  if (_socket >= 0)
    if (occ->id() == OccurrenceId::SequencerDone) {
      _control.set_target_state(PartitionControl::Running);
      return 0;
    }

  return occ; 
}

InDatagram* RemoteSeqApp::events     (InDatagram* dg) 
{ 
  if (_socket >= 0) {
    int id   = dg->datagram().seq.service();
    int info = _last_run.run() | (_last_run.experiment()<<16);
    if (dg->datagram().xtc.damage.value()!=0) {
      info = -id;
      ::write(_socket,&info,sizeof(info));
    }
    else if (_wait_for_configure) {
      if (id==TransitionId::Configure) {
  ::write(_socket,&info,sizeof(info));
  _wait_for_configure = false;
      }
    }
    else if (id==TransitionId::Enable ||
       id==TransitionId::EndCalibCycle)
      ::write(_socket,&info,sizeof(info));
  }
  return dg;
}
