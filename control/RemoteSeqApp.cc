#include "RemoteSeqApp.hh"
#include "StateSelect.hh"
#include "ConfigSelect.hh"
#include "PartitionSelect.hh"
#include "PVManager.hh"
#include "RemotePartition.hh"
#include "RunStatus.hh"
#include "RemoteSeqResponse.hh"

#include "pdsapp/config/GlobalCfg.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/EnableEnv.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pds/service/Task.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"
#include "pds/config/ControlConfigType.hh"
#include "pds/config/PartitionConfigType.hh"
#include "pds/client/L3TIterator.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>

//#define DBUG

static const int MaxConfigSize = 0x100000;
static const int Control_Port  = 10130;

using namespace Pds;
using Pds_ConfigDb::GlobalCfg;

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

RemoteSeqApp::MsgType RemoteSeqApp::readTransition()
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
    return INVALID;
  }
  int payload = config._sizeof()-sizeof(config);
  if (payload>0) {
    len = ::recv(_socket, &config+1, payload, MSG_WAITALL);
    if (len != payload) {
      printf("RemoteSeqApp failed to read config payload(%d/%d) : %s\n",
       len,payload,strerror(errno));
      return INVALID;
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
  else if (config.uses_l3t_events())
    printf("received remote configuration for %d l3t_events, %d controls\n",
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
  return TRANSITION;
}

RemoteSeqApp::MsgType RemoteSeqApp::processTransitionCmd(RemoteSeqCmd& cmd)
{
  switch (cmd.u32Type)
  {
  case RemoteSeqCmd::CMD_GET_CUR_EVENT_NUM:
    { int64_t iEventNum = _runStatus.getEventNum();
      ::write(_socket,&iEventNum,sizeof(iEventNum));
#ifdef DBUG
      printf("RemoteSeqApp get events [%ld]\n",long(iEventNum));
#endif
    } break;
  case RemoteSeqCmd::CMD_GET_CUR_L3EVENT_NUM:
    { int64_t iEventNum = _runStatus.getL3EventNum();
      ::write(_socket,&iEventNum,sizeof(iEventNum));
#ifdef DBUG
      printf("RemoteSeqApp get l3events [%ld]\n",long(iEventNum));
#endif
    } break;
  default:
    printf("RemoteSeqApp unrecognized command [%u]\n",cmd.u32Type);
    break;
  }

  return EVENTNUM; // keep DAQ running
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

            _control.set_target_state(PartitionControl::Mapped);
            _control.wait_for_target();
            _control.release_target();

            _socket = s;

            //  First, send the current configdb and run key
            length = strlen(_control.partition().dbpath());
            ::write(_socket,&length,sizeof(length));
            ::write(_socket,_control.partition().dbpath(),length);

            uint32_t old_key = _control.get_transition_env(TransitionId::Configure);
            uint32_t options = old_key&DbKeyMask;
            if (lrecord) options |= RecordValMask;
            ::write(_socket,&options,sizeof(options));

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
              if (palloc->l3tag())
                partition->set_l3t(palloc->l3path(),
                                   palloc->l3veto(),
                                   palloc->unbiased_fraction());
              else
                partition->clear_l3t();
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
                printf("l3tag %c  veto %c  path %s\n",
                       partition->l3tag()?'t':'f',
                       partition->l3veto()?'t':'f',
                       partition->l3path());
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
                PartitionConfigType* apcfg;
                Allocation* alloc = new Allocation(_control.partition());
                {
                  const PartitionConfigType& pcfg = 
                    *reinterpret_cast<const PartitionConfigType*>
                    (GlobalCfg::instance().fetch(_partitionConfigType));
                  ndarray<Partition::Source,1> psrcs = make_ndarray<Partition::Source>(pcfg.numSources());
                  unsigned pnsrcs=0;

                  const QList<DetInfo>& detectors = _pselect.detectors();
                  const QList<ProcInfo>& segments = _pselect.segments ();
                  if (partition->l3tag())
                    alloc->set_l3t(partition->l3path(),
                                   partition->l3veto(),
                                   partition->l3uf  ());
                  else
                    alloc->clear_l3t();
                  for(unsigned j=0; j<partition->nodes(); j++) {
                    RemoteNode& node = *partition->node(j);
                    for(int k=0; k<detectors.size(); k++) {
                      if (detectors[k].phy() == node.phy()) {
                        if (!node.readout())
                          alloc->remove(segments[k]);
                        else {
                          alloc->node(segments[k])->setTransient(!node.record());
                          if (node.record())
                            for(unsigned i=0; i<pcfg.numSources(); i++)
                              if (pcfg.sources()[i].src().phy()==node.phy())
                                psrcs[pnsrcs++] = pcfg.sources()[i];
                        }
                      }
                    }
                  }
                  apcfg = new (new char[pcfg._sizeof()])
                    PartitionConfigType(pcfg.numWords(),pnsrcs,pcfg.bldMask().data(),psrcs.data());
                }

#ifdef DBUG
                printf("Reallocating with partition %p\n",alloc);
                printf("l3tag %c  veto %c  path %s\n",
                       alloc->l3tag()?'t':'f',
                       alloc->l3veto()?'t':'f',
                       alloc->l3path());
#endif

                _control.set_partition(*alloc, apcfg);
                _control.set_target_state(PartitionControl::Unmapped);
                delete alloc;
              }
              _control.wait_for_target();
              _control.set_target_state(PartitionControl::Mapped);
              _control.release_target();
              //
              //  Reconfigure with the initial settings
              //
              if (readTransition() == INVALID)  break;

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
                MsgType msg_type = readTransition();
                if (msg_type == INVALID)  break;
                else if (msg_type != TRANSITION) continue;
                //
                //  A request for a cycle of zero duration is an EndCalib
                //
                if (config.uses_duration() && config.duration()==EndCalibTime) {
                  _control.set_target_state(PartitionControl::Running);
                }
                else if (config.uses_duration() && config.duration()==EndRunTime) {
                  _control.set_target_state(PartitionControl::Configured);
                }
                else {
                  _control.wait_for_target();
                  if (config.uses_duration())
                    _control.set_transition_env(TransitionId::BeginCalibCycle,
                                                EnableEnv(config.duration()).value());
                  else if (config.uses_l3t_events())
                    _control.set_transition_env(TransitionId::BeginCalibCycle,
                                                EnableEnv(-1).value());
                  else if (config.uses_events())
                    _control.set_transition_env(TransitionId::BeginCalibCycle,
                                                EnableEnv(config.events()).value());
                  else
                    ;
                  _control.set_transition_payload(TransitionId::BeginCalibCycle,&_configtc,_config_buffer);
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
              const PartitionConfigType& pcfg = 
                *reinterpret_cast<const PartitionConfigType*>(GlobalCfg::instance().fetch(_partitionConfigType));
              char* p = new char[pcfg._sizeof()];
              _control.set_partition(*palloc,new(p) PartitionConfigType(pcfg));
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
            _control.set_transition_env    (TransitionId::BeginCalibCycle,
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
    switch (occ->id()) {
    case OccurrenceId::SequencerDone:
      _control.set_target_state(PartitionControl::Running);
      return 0;
    case OccurrenceId::EvrEnabled: {
      RemoteSeqResponse response(_last_run.experiment(),
                                 _last_run.run(),
                                 Pds::TransitionId::Enable, 0);
      ::write(_socket,&response,sizeof(response));
      return 0;
    }
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
    if (id==TransitionId::L1Accept) {
      ControlConfigType& config =
        *reinterpret_cast<ControlConfigType*>(_config_buffer);
      if (config.uses_l3t_events()) {
        L3TIterator it;
        it.iterate(&dg->datagram().xtc);
        if (it.pass() && ++_l3t_events == config.events())
          _control.set_target_state(PartitionControl::Running);
      }
      return 0;
    }

    RemoteSeqResponse response(_last_run.experiment(),
                               _last_run.run(),
                               dg->datagram().seq.service(),
                               dg->datagram().xtc.damage.value());

    if (dg->datagram().xtc.damage.value()!=0)
      ::write(_socket,&response,sizeof(response));
    else if (_wait_for_configure) {
      if (id==TransitionId::Configure) {
        ::write(_socket,&response,sizeof(response));
        _wait_for_configure = false;
      }
    }
    else if (id==TransitionId::EndCalibCycle)
      ::write(_socket,&response,sizeof(response));
  }
  return dg;
}
