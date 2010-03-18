#include "RemoteSeqApp.hh"
#include "StateSelect.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pds/service/Task.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const int MaxConfigSize = 0x100000;
static const int Control_Port  = 10130;

using namespace Pds;

RemoteSeqApp::RemoteSeqApp(PartitionControl& control,
			   StateSelect&      manual,
			   const Src&        src) :
  _control      (control),
  _manual       (manual),
  _configtc     (_controlConfigType, src),
  _config_buffer(new char[MaxConfigSize]),
  _task         (new Task(TaskObject("remseq"))),
  _port         (Control_Port + control.header().platform()),
  _socket       (-1)
{
  _task->call(this);
}

RemoteSeqApp::~RemoteSeqApp()
{
  _task->destroy();
  delete[] _config_buffer;
}

bool RemoteSeqApp::readTransition()
{
  Pds::ControlData::ConfigV1& config = 
    *reinterpret_cast<Pds::ControlData::ConfigV1*>(_config_buffer);

  int len = ::recv(_socket, &config, sizeof(config), MSG_WAITALL);
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
    else if (config.uses_duration())
      printf("received remote configuration for %d/%d seconds, %d controls\n",
	     config.duration().seconds(),
	     config.duration().nanoseconds(),
	     config.npvControls());
    else if (config.uses_events())
      printf("received remote configuration for %d events, %d controls\n",
	     config.events(),
	     config.npvControls());
  }
  _configtc.extent = sizeof(Xtc) + config.size();
  return true;
}

void RemoteSeqApp::routine()
{
  Pds::ControlData::ConfigV1& config = 
    *reinterpret_cast<Pds::ControlData::ConfigV1*>(_config_buffer);

  //  replace the configuration with default running
  new(_config_buffer) ControlConfigType(Pds::ControlData::ConfigV1::Default);
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
	  printf("RemoteSeqApp accepted connection from %x/%d\n",
		 name.get().address(), name.get().portId());

	  _manual.disable_control();

	  //  First, reconfigure with the initial settings
	  if (readTransition()) {
	    _control.set_transition_payload(TransitionId::Configure,&_configtc,_config_buffer);

	    _wait_for_configure = true;
	    _control.reconfigure();

	    while(1) {
	      if (!readTransition())  break;
	      _control.set_transition_payload(TransitionId::BeginCalibCycle,&_configtc,_config_buffer);
	      _control.set_transition_env(TransitionId::Enable, 
					  config.uses_duration() ?
					  EnableEnv(config.duration()).value() :
					  EnableEnv(config.events()).value());
	      _control.set_target_state(PartitionControl::Enabled);
	    }
	  }

	  close(_socket);
	  _socket = -1;

	  //  replace the configuration with default running
	  new(_config_buffer) ControlConfigType(Pds::ControlData::ConfigV1::Default);
	  _configtc.extent = sizeof(Xtc) + config.size();
	  _control.set_transition_payload(TransitionId::Configure,&_configtc,_config_buffer);
	  _control.set_transition_env(TransitionId::Enable, 
				      config.uses_duration() ?
				      EnableEnv(config.duration()).value() :
				      EnableEnv(config.events()).value());

	  _manual.enable_control();
	  _control.set_target_state(PartitionControl::Configured);

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
  return tr;
}

Occurrence* RemoteSeqApp::occurrences(Occurrence* occ) 
{
  if (occ->id() == OccurrenceId::SequencerDone) {
    _control.set_target_state(PartitionControl::Running);
    return 0;
  }
  return occ; 
}

InDatagram* RemoteSeqApp::events     (InDatagram* dg) 
{ 
  if (_socket >= 0) {
    int id = dg->datagram().seq.service();
    if (dg->datagram().xtc.damage.value()!=0) {
      id = -id;
      ::write(_socket,&id,sizeof(id));
    }
    else if (_wait_for_configure) {
      if (id==TransitionId::Configure) {
	::write(_socket,&id,sizeof(id));
	_wait_for_configure = false;
      }
    }
    else if (id==TransitionId::Enable ||
	     id==TransitionId::Disable)
      ::write(_socket,&id,sizeof(id));
  }
  return dg;
}
