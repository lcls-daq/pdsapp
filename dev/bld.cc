//
//  How to put valid BLD in the configure return datagram?
//
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/EventStreams.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/NullServer.hh"
#include "pds/utility/EbBase.hh"
#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

// BldStreams
#include "pds/utility/EbS.hh"
#include "pds/utility/EbSequenceKey.hh"
#include "pds/utility/ToEventWire.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/BitList.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/bld/bldData.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/BldInfo.hh"
// Bld from XRT cameras
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pdsdata/camera/FrameV1.hh"

#include "pds/config/EvrConfigType.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static const unsigned MAX_EVENT_SIZE = 4*1024*1024;
static const unsigned NetBufferDepth = 32;

namespace Pds {

  static NullServer* _evrServer = 0;

  class BldConfigApp : public Appliance {
  public:
    BldConfigApp(const Src& src) :
      _configtc       (TypeId(TypeId::Id_Xtc,1), src),
      _config_payload (0) {}
    ~BldConfigApp() { if (_config_payload) delete[] _config_payload; }
  public:
    InDatagram* events     (InDatagram* dg) {
      if (dg->datagram().seq.service()==TransitionId::Configure)
        dg->insert(_configtc, _config_payload);
      return dg; 
    }

    Transition* transitions(Transition* tr) {
      if (tr->id()==TransitionId::Map) {
        const Allocation& alloc = reinterpret_cast<const Allocate*>(tr)->allocation(); 
        unsigned m = alloc.bld_mask();
#define CheckType(bldType)  (m & (1<<BldInfo::bldType))
#define SizeType(dataType)  (sizeof(Xtc) + sizeof(dataType))
#define AddType(bldType,idType,dataType) {                                                                       \
        Xtc& xtc = *new(p) Xtc(TypeId(TypeId::idType,(uint32_t)dataType::version), BldInfo(0,BldInfo::bldType)); \
        xtc.extent = SizeType(dataType);                                                                         \
        p += xtc.extent;                                                                                         \
      }
#define SizeCamType         (sizeof(Xtc)*3 + sizeof(TM6740ConfigType) + sizeof(FrameFexConfigType) + sizeof(PimImageConfigType))
#define AddCamXtc(bldType,idType,dataType) {                            \
        Xtc& xtc = *new(p) Xtc(idType, BldInfo(0,BldInfo::bldType));    \
        xtc.extent += sizeof(dataType);                                 \
        p += xtc.extent;                                                \
      }
#define AddCamType(bldType) {                                           \
        AddCamXtc(bldType,_tm6740ConfigType  ,TM6740ConfigType);        \
        AddCamXtc(bldType,_frameFexConfigType,FrameFexConfigType);      \
        AddCamXtc(bldType,_pimImageConfigType,PimImageConfigType);      \
      }
      unsigned extent = 0;
      if (CheckType(EBeam))           extent += SizeType(BldDataEBeam);
      if (CheckType(PhaseCavity))     extent += SizeType(BldDataPhaseCavity);
      if (CheckType(FEEGasDetEnergy)) extent += SizeType(BldDataFEEGasDetEnergy);
      if (CheckType(Nh2Sb1Ipm01))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HxxUm6Imb01))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HxxUm6Imb02))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HfxDg2Imb01))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HfxDg2Imb02))     extent += SizeType(BldDataIpimb);   
      if (CheckType(XcsDg3Imb03))     extent += SizeType(BldDataIpimb);   
      if (CheckType(XcsDg3Imb04))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HfxDg3Imb01))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HfxDg3Imb02))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HxxDg1Cam  ))     extent += SizeCamType;
      if (CheckType(HfxDg2Cam  ))     extent += SizeCamType;
      if (CheckType(HfxDg3Cam  ))     extent += SizeCamType;
      if (CheckType(XcsDg3Cam  ))     extent += SizeCamType;
      if (CheckType(HfxMonCam  ))     extent += SizeCamType;
      if (CheckType(HfxMonImb01))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HfxMonImb02))     extent += SizeType(BldDataIpimb);   
      if (CheckType(HfxMonImb03))     extent += SizeType(BldDataIpimb);
      if (CheckType(MecLasEm01))      extent += SizeType(BldDataIpimb);
      if (CheckType(MecTctrPip01))    extent += SizeType(BldDataIpimb);
      if (CheckType(MecTcTrDio01))    extent += SizeType(BldDataIpimb);
      if (CheckType(MecXt2Ipm02))     extent += SizeType(BldDataIpimb);
      if (CheckType(MecXt2Ipm03))     extent += SizeType(BldDataIpimb);
      if (CheckType(MecHxmIpm01))     extent += SizeType(BldDataIpimb);
      if (CheckType(GMD))             extent += SizeType(BldDataGMD);
      
      _configtc.extent = sizeof(Xtc)+extent;
      if (extent) {
        if (_config_payload) delete[] _config_payload;
        char* p = _config_payload = new char[extent];
        if (CheckType(EBeam))           AddType(EBeam,           Id_EBeam,           BldDataEBeam);
        if (CheckType(PhaseCavity))     AddType(PhaseCavity,     Id_PhaseCavity,     BldDataPhaseCavity);
        if (CheckType(FEEGasDetEnergy)) AddType(FEEGasDetEnergy, Id_FEEGasDetEnergy, BldDataFEEGasDetEnergy );
        if (CheckType(Nh2Sb1Ipm01))     AddType(Nh2Sb1Ipm01,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HxxUm6Imb01))     AddType(HxxUm6Imb01,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HxxUm6Imb02))     AddType(HxxUm6Imb02,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HfxDg2Imb01))     AddType(HfxDg2Imb01,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HfxDg2Imb02))     AddType(HfxDg2Imb02,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(XcsDg3Imb03))     AddType(XcsDg3Imb03,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(XcsDg3Imb04))     AddType(XcsDg3Imb04,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HfxDg3Imb01))     AddType(HfxDg3Imb01,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HfxDg3Imb02))     AddType(HfxDg3Imb02,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HxxDg1Cam  ))     AddCamType(HxxDg1Cam);
        if (CheckType(HfxDg2Cam  ))     AddCamType(HfxDg2Cam);
        if (CheckType(HfxDg3Cam  ))     AddCamType(HfxDg3Cam);
        if (CheckType(XcsDg3Cam  ))     AddCamType(XcsDg3Cam);
        if (CheckType(HfxMonCam  ))     AddCamType(HfxMonCam);
        if (CheckType(HfxMonImb01))     AddType(HfxMonImb01,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HfxMonImb02))     AddType(HfxMonImb02,     Id_SharedIpimb,     BldDataIpimb); 
        if (CheckType(HfxMonImb03))     AddType(HfxMonImb03,     Id_SharedIpimb,     BldDataIpimb);
        if (CheckType(MecLasEm01))      AddType(MecLasEm01,      Id_SharedIpimb,     BldDataIpimb);
        if (CheckType(MecTctrPip01))     AddType(MecTctrPip01,    Id_SharedIpimb,     BldDataIpimb);
        if (CheckType(MecTcTrDio01))    AddType(MecTcTrDio01,    Id_SharedIpimb,     BldDataIpimb);
        if (CheckType(MecXt2Ipm02))     AddType(MecXt2Ipm02,     Id_SharedIpimb,     BldDataIpimb);
        if (CheckType(MecXt2Ipm03))     AddType(MecXt2Ipm03,     Id_SharedIpimb,     BldDataIpimb);
        if (CheckType(MecHxmIpm01))     AddType(MecHxmIpm01,     Id_SharedIpimb,     BldDataIpimb);
        if (CheckType(GMD))             AddType(GMD,             Id_GMD,             BldDataGMD);
      }
#undef CheckType
#undef SizeType
#undef AddType
      }
      return tr; 
    }
  private:
    Xtc      _configtc;
    char*    _config_payload;
  };

  class BldDbg : public Appliance {
    enum { Period=300 };
  public:
    BldDbg(EbBase* eb) : _eb(eb), _cnt(0) {}
    ~BldDbg() {}
  public:
    InDatagram* events(InDatagram* dg) 
    {
      if (_cnt!=0) _cnt--;
      if ((dg->datagram().xtc.damage.value()&(1<<Damage::DroppedContribution)) && _cnt==0) {
  _eb->dump(1);
  _cnt = Period;
      }
      return dg; 
    }
    Transition* transitions(Transition* tr) 
    {
      if (tr->id() == TransitionId::Disable)
  _eb->dump(1);
      return tr; 
    }
  private:
    EbBase*  _eb;
    unsigned _cnt;
  };

  class BldCallback : public EventCallback, 
          public SegWireSettings {
  public:
    BldCallback(Task*      task,
    unsigned   platform,
    unsigned   mask) :
      _task    (task),
      _platform(platform)
    {
      Node node(Level::Source,platform);
      _sources.push_back(DetInfo(node.pid(), 
         DetInfo::BldEb, 0,
         DetInfo::NoDevice, 0));
      for(unsigned i=0; i<BldInfo::NumberOf; i++)
        if ( (1<<i)&mask ) 
          _sources.push_back(BldInfo(node.pid(),(BldInfo::Type)i));
    }

    virtual ~BldCallback() { _task->destroy(); }

  public:    
    // Implements SegWireSettings
    void connect (InletWire& inlet, StreamParams::StreamType s, int ip) {}
    const std::list<Src>& sources() const { return _sources; }
  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams) 
    { 
      Inlet* inlet = streams.stream()->inlet();
      (new BldDbg(static_cast<EbBase*>(streams.wire())))->connect(inlet);
      (new BldConfigApp(_sources.front()))->connect(inlet);
    }
    void failed(Reason reason)
    {
      static const char* reasonname[] = { "platform unavailable", 
            "crates unavailable", 
            "fcpm unavailable" };
      printf("SegTest: unable to allocate crates on platform 0x%x : %s\n", 
       _platform, reasonname[reason]);
      delete this;
    }
    void dissolved(const Node& who)
    {
      const unsigned userlen = 12;
      char username[userlen];
      Node::user_name(who.uid(),username,userlen);
      
      const unsigned iplen = 64;
      char ipname[iplen];
      Node::ip_name(who.ip(),ipname, iplen);
      
      printf("SegTest: platform 0x%x dissolved by user %s, pid %d, on node %s", 
       who.platform(), username, who.pid(), ipname);
      
      delete this;
    }
  private:
    Task*          _task;
    unsigned       _platform;
    std::list<Src> _sources;
  };

  class BldEvBuilder : public EbS {
  public:
    BldEvBuilder(const Src& id,
     const TypeId& ctns,
     Level::Type level,
     Inlet& inlet,
     OutletWire& outlet,
     int stream,
     int ipaddress,
     unsigned eventsize,
     unsigned eventpooldepth,
     VmonEb* vmoneb=0) : 
      EbS(id, ctns, level, inlet, outlet, stream, ipaddress, eventsize, eventpooldepth, vmoneb) {}
    ~BldEvBuilder() {}
  public:
    int processIo(Server* s) { EbS::processIo(s); return 1; }
    int poll() {   
      if(!ServerManager::poll()) return 0;
      if(active().isZero()) ServerManager::arm(managed());
      return 1;
    }
  private:
    void _flushOne() {
      EbEventBase* event = _pending.forward();
      EbEventBase* empty = _pending.empty();
      //  Prefer to flush an unvalued event first
      while( event != empty ) {
  EbBitMask value(event->allocated().remaining() & _valued_clients);
  if (value.isZero()) {
    _postEvent(event);
    return;
  }
  event = event->forward();
      }
      _postEvent(_pending.forward());
    }
    EbEventBase* _new_event  ( const EbBitMask& serverId) {
      unsigned depth = _datagrams.depth();

      if (_vmoneb) _vmoneb->depth(depth);
      
      if (depth<=1) _flushOne(); // keep one buffer for recopy possibility
      
      CDatagram* datagram = new(&_datagrams) CDatagram(_ctns, _id);
      EbSequenceKey* key = new(&_keys) EbSequenceKey(const_cast<Datagram&>(datagram->datagram()));
      return new(&_events) EbEvent(serverId, _clients, datagram, key);
    }
    EbEventBase* _new_event  ( const EbBitMask& serverId,
             char*            payload, 
             unsigned         sizeofPayload ) {
      CDatagram* datagram = new(&_datagrams) CDatagram(_ctns, _id);
      EbSequenceKey* key = new(&_keys) EbSequenceKey(const_cast<Datagram&>(datagram->datagram()));
      EbEvent* event = new(&_events) EbEvent(serverId, _clients, datagram, key);
      event->allocated().insert(serverId);
      event->recopy(payload, sizeofPayload, serverId);
      
      unsigned depth = _datagrams.depth();
      
      if (_vmoneb) _vmoneb->depth(depth);
      
      if (depth<=1) _flushOne();
      
      return event;
    }
  private:
    IsComplete   _is_complete( EbEventBase* event, const EbBitMask& serverId)
    {
      //
      //  Check for special case of readout event without beam => no BLD expected
      //    Search for FIFO Event with current pulseId and beam present code
      //
      if (_evrServer) {
  if (serverId.hasBitSet(_evrServer->id())) {  // EVR just added 
    Datagram* dg = event->datagram();
    uint32_t timestamp = dg->seq.stamp().fiducials();
    const Xtc& xtc  = *reinterpret_cast<const Xtc*>(_evrServer->payload());
    const Xtc& xtc1 = *reinterpret_cast<const Xtc*>(xtc.payload());
    const EvrDataType& evrd = *reinterpret_cast<const EvrDataType*>(xtc1.payload());

//           static int nprint=0;
//    if (nprint++%119 == 0) {
//      printf("== nfifo %d\n",evrd.numFifoEvents());
//      for(unsigned i=0; i<evrd.numFifoEvents(); i++) {
//        const EvrDataType::FIFOEvent& fe = evrd.fifoEvent(i);
//        printf("  %d : %08x/%08x : %d\n", 
//         i, fe.TimestampHigh, fe.TimestampLow, fe.EventCode);
//      }
//    }

    for(unsigned i=0; i<evrd.numFifoEvents(); i++) {
      const EvrDataType::FIFOEvent& fe = evrd.fifoEvent(i);
      if (fe.TimestampHigh == timestamp &&
    fe.EventCode >= 140 &&
    fe.EventCode <= 146)
        return EbS::_is_complete(event,serverId);  //  A beam-present code is found
    }
    return NoBuild;   // No beam-present code is found
  }
      }
      return EbS::_is_complete(event,serverId);   //  Not only EVR is present
    }
  };

  class BldStreams : public WiredStreams {
  public:
    BldStreams(PartitionMember& m) :
      WiredStreams(VmonSourceId(m.header().level(), m.header().ip()))
    {
      unsigned max_size = MAX_EVENT_SIZE;
      unsigned net_buf_depth = 16;
#ifdef BLD_DELAY
      unsigned eb_depth = 120;
#else
      unsigned eb_depth = 8;
#endif

      const Node& node = m.header();
      Level::Type level = node.level();
      int ipaddress = node.ip();
      const Src& src = m.header().procInfo();
      for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

  _outlets[s] = new ToEventWire(*stream(s)->outlet(), 
              m, 
              ipaddress, 
              max_size*net_buf_depth,
              m.occurrences());

  _inlet_wires[s] = new BldEvBuilder(src,
             _xtcType,
             level,
             *stream(s)->inlet(), 
             *_outlets[s],
             s,
             ipaddress,
             max_size, eb_depth,
             new VmonEb(src,32,eb_depth,(1<<23),(1<<22)));
               
  (new VmonServerAppliance(src))->connect(stream(s)->inlet());
      }
    }
    ~BldStreams() {  
      for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
  delete _inlet_wires[s];
  delete _outlets[s];
      }
    }
  };

  class BldSegmentLevel : public SegmentLevel {
  public:
    BldSegmentLevel(unsigned     platform, 
                    BldCallback& cb) :
      SegmentLevel(platform, cb, cb, 0) {}
    ~BldSegmentLevel() {}
  public:
    bool attach() {
      start();
      if (connect()) {
        _streams = new BldStreams(*this);  // specialized here
        _streams->connect();

        _callback.attached(*_streams);

        //  Add the L1 Data servers  
        _settings.connect(*_streams->wire(StreamParams::FrameWork),
                          StreamParams::FrameWork, 
                          header().ip());

        //    Message join(Message::Ping);
        //    mcast(join);
        _reply.ready(true);
        mcast(_reply);
        return true;
      } else {
        _callback.failed(EventCallback::PlatformUnavailable);
        return false;
      }
    }
    void allocated(const Allocation& alloc, unsigned index) {
      //  add segment level EVR
      unsigned partition = alloc.partitionid();
      unsigned nnodes    = alloc.nnodes();
      unsigned bldmask   = alloc.bld_mask();
      InletWire & inlet  = *_streams->wire(StreamParams::FrameWork);

      for (unsigned n = 0; n < nnodes; n++) {
        const Node & node = *alloc.node(n);
        if (node.level() == Level::Segment) {
          Ins ins = StreamPorts::event(partition, Level::Observer, 0, 0);
          _evrServer =
            new NullServer(ins,
                           header().procInfo(),
                           sizeof(Pds::EvrData::DataV3)+256*sizeof(Pds::EvrData::DataV3::FIFOEvent),
                           NetBufferDepth);

          Ins mcastIns(ins.address());
          _evrServer->server().join(mcastIns, Ins(header().ip()));

          inlet.add_input(_evrServer);
          _inputs.push_back(_evrServer);
          break;
        }
      }

      for(int i=0; i<BldInfo::NumberOf; i++) {
        if (bldmask & (1<<i)) {
          Node node(Level::Reporter, 0);
          node.fixup(StreamPorts::bld(i).address(),Ether());
          Ins ins( node.ip(), StreamPorts::bld(0).portId());
          BldServer* srv = new BldServer(ins, BldInfo(0,(BldInfo::Type)i), MAX_EVENT_SIZE);
          inlet.add_input(srv);
          _inputs.push_back(srv);
          srv->server().join(ins, header().ip());
          printf("Bld::allocated assign bld  fragment %d  %x/%d\n",
                 srv->id(),ins.address(),srv->server().portId());
        }
      } 

      unsigned vectorid = 0;

      for (unsigned n = 0; n < nnodes; n++) {
        const Node & node = *alloc.node(n);
        if (node.level() == Level::Event) {
          // Add vectored output clients on inlet
          Ins ins = StreamPorts::event(partition,
                                       Level::Event,
                                       vectorid,
                                       index);
          InletWireIns wireIns(vectorid, ins);
          inlet.add_output(wireIns);
          printf("SegmentLevel::allocated adding output %d to %x/%d\n",
                 vectorid, ins.address(), ins.portId());
          vectorid++;
        }
      }
      OutletWire* owire = _streams->stream(StreamParams::FrameWork)->outlet()->wire();
      owire->bind(OutletWire::Bcast, StreamPorts::bcast(partition, 
                                                        Level::Event,
                                                        index));
    }
    void dissolved() {
      _evrServer = 0;
      for(std::list<Server*>::iterator it=_inputs.begin(); it!=_inputs.end(); it++)
        static_cast <InletWireServer*>(_streams->wire())->remove_input(*it);
      _inputs.clear();

      static_cast <InletWireServer*>(_streams->wire())->remove_outputs();
    }

  private:
    std::list<Server*> _inputs;
  };
}

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -p <platform> [-m <mask>]\n",p);
}

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = unsigned(-1);
  // parse the command line for our boot parameters
  unsigned platform = NO_PLATFORM;
  unsigned mask = (1<<BldInfo::NumberOf)-1;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "p:m:")) != EOF ) {
    switch(c) {
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'm':
      mask = strtoul(optarg, NULL, 0);
      break;
    default:
      usage(argv[0]);
      return 0;
    } 
 }

  if (platform==NO_PLATFORM) {
    usage(argv[0]);
    return 0;
  }

  Task*               task = new Task(Task::MakeThisATask);
  BldCallback*          cb = new BldCallback(task, platform, mask);
  BldSegmentLevel* segment = new BldSegmentLevel(platform, *cb);
  if (segment->attach())
    task->mainLoop();

  segment->detach();
  return 0;
}
