//
//  How to put valid BLD in the configure return datagram?
//
//    Maintain map of last seen configure data
//    Persist last seen configure data in the local file system (/tmp)
//    Read at initialization
//    When configure data changes, force a reconfigure and drop data until.
//
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/EventStreams.hh"

#include "pds/client/FrameCompApp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/BldServerTransient.hh"
#include "pds/utility/NullServer.hh"
#include "pds/utility/EbBase.hh"
#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

// BldStreams
#include "pds/utility/EbS.hh"
#include "pds/utility/EbSequenceKey.hh"
#include "pds/utility/ToEventWireScheduler.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/BitList.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/psddl/bld.ddl.h"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/Level.hh"
// Bld from XRT cameras
#include "pds/config/AcqConfigType.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pdsdata/psddl/camera.ddl.h"

#include "pds/config/EvrConfigType.hh"

#include <map>

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

//#define DBUG

typedef Pds::Bld::BldDataEBeamV6 BldDataEBeam;
typedef Pds::Bld::BldDataFEEGasDetEnergyV1 BldDataFEEGasDetEnergy;
typedef Pds::Bld::BldDataIpimbV1 BldDataIpimb;
typedef Pds::Bld::BldDataGMDV2 BldDataGMD;
typedef Pds::Bld::BldDataSpectrometerV1 SpectrometerType;
static const Pds::TypeId::Type Id_SpectrometerType = 
  Pds::TypeId::Type(SpectrometerType::TypeId);
static Pds::TypeId _spectrometerType(Id_SpectrometerType,
                                     SpectrometerType::Version);

//    typedef BldDataAcqADCV1 BldDataAcqADC;
using Pds::Bld::BldDataPhaseCavity;
using Pds::Bld::BldDataPimV1;

static const unsigned MAX_EVENT_SIZE = 8*1024*1024;
#ifdef BLD_DELAY
static const unsigned eb_depth = 64;
#else
static const unsigned eb_depth = 32;
#endif
static const unsigned net_buf_depth = 16;
static const unsigned EvrBufferDepth = 32;
static const unsigned AppBufferDepth = eb_depth + 16;  // Handle
static const char _tc_init[] = { 0*1024 };
static const FrameFexConfigType _frameFexConfig(FrameFexConfigType::FullFrame, 1,
                                                FrameFexConfigType::NoProcessing,
                                                Pds::Camera::FrameCoord(0,0),
                                                Pds::Camera::FrameCoord(0,0),
                                                0, 0, 0);

namespace Pds {

  static NullServer* _evrServer = 0;

  class BldConfigCache {
  public:
    BldConfigCache() : _path("/var/tmp") {}
    ~BldConfigCache() {}
  public:
    char* fetch(const Xtc& tc) {
      uint64_t key = tc.contains.value();
      key = (key<<32) | tc.src.phy();
      if (_map.find(key)==_map.end()) {
        char fname[64];
        sprintf(fname,"%s/%08x.%08x",_path,unsigned(key>>32),unsigned(key&0xffffffff));
        FILE* f = fopen(fname,"r");
        if (!f) {
          printf("Failed to open %s\n",fname);
          return 0;
        }
        printf("Opened %s\n",fname);
        unsigned len = tc.sizeofPayload();
        char* c = new char[len];
        fread(c,len,1,f);
        fclose(f);
        _map[key] = Entry(len,c);
      }
      return _map[key].payload;
    }

    void update(const Xtc& tc, const char* payload, unsigned len) {
      uint64_t key = tc.contains.value();
      key = (key<<32) | tc.src.phy();
      char* p = new char[len];
      memcpy(p,payload,len);
      _map[key] = Entry(len,p);
    }

    void store() {
      for(std::map<uint64_t,Entry>::iterator it=_map.begin(); it!=_map.end(); it++) {
        uint64_t key = it->first;
        char fname[64];
        sprintf(fname,"%s/%08x.%08x",_path,unsigned(key>>32),unsigned(key&0xffffffff));
        FILE* f = fopen(fname,"w");
        if (!f) continue;
        fwrite(it->second.payload,it->second.extent,1,f);
        fclose(f);
      }
    }
  private:
    class Entry {
    public:
      Entry() : extent(0), payload(0) {}
      Entry(unsigned e, char* p) : extent(e), payload(p) {}
      ~Entry() { if (payload) delete[] payload; }
      Entry& operator=(const Entry& o) {
  memcpy(payload=new char[extent=o.extent],o.payload,o.extent);
  return *this; }
      unsigned extent; char* payload;
    };
    const char*               _path;
    std::map<uint64_t, Entry> _map;
  };

  //
  //  Receive the BLD data types and split them into their primitve components.
  //  Trim redundant configuration data and trap illegal configuration changes.
  //
  class BldParseApp : public Appliance, public XtcIterator {
  public:
    BldParseApp(BldConfigCache& cache) :
      _pool (MAX_EVENT_SIZE, AppBufferDepth),
      _occ  (sizeof(UserMessage),4),
      _cache(cache),
      _pid  (getpid())
    {}
    ~BldParseApp() {}
  public:
    InDatagram* events     (InDatagram* dg) {
      if (dg->seq.isEvent()) {
        _dg = new(&_pool) CDatagram(*dg);
        iterate(&dg->xtc);
        if (_reconfigure_requested) {
          delete _dg;
          return 0;
        }
        return _dg;
      }
      else if (dg->seq.service()==TransitionId::Configure) {
  //  Reset flag
  _reconfigure_requested = false;
  //  Map from BldInfo enumeration to Configuration data type
  uint64_t
    EBeamMask       = 1ULL<<BldInfo::EBeam,
    PhaseCavityMask = 1ULL<<BldInfo::PhaseCavity,
    FEEGasDetMask   = 1ULL<<BldInfo::FEEGasDetEnergy,
    GMDMask         = 1ULL<<BldInfo::GMD;
  uint64_t IpimbMask =
    ((1ULL<<(BldInfo::HfxDg3Imb02+1)) - (1ULL<<BldInfo::Nh2Sb1Ipm01)) |
    ((1ULL<<(BldInfo::MecHxmIpm01+1)) - (1ULL<<BldInfo::HfxMonImb01)) |
    ((1ULL<<(BldInfo::CxiDg4Imb01+1)) - (1ULL<<BldInfo::CxiDg1Imb01)) |
    ((1ULL<<(BldInfo::MecXt2Pim03+1)) - (1ULL<<BldInfo::XppMonPim0))  |
    ((1ULL<<(BldInfo::Nh2Sb1Ipm02+1)) - (1ULL<<BldInfo::Nh2Sb1Ipm02)) |
    ((1ULL<<(BldInfo::XcsLamIpm01+1)) - (1ULL<<BldInfo::XcsUsrIpm01));
  uint64_t PimMask =
    ((1ULL<<(BldInfo::HfxMonCam+1)) - (1ULL<<BldInfo::HxxDg1Cam)) |
    ((1ULL<<(BldInfo::CxiDg4Pim+1)) - (1ULL<<BldInfo::CxiDg1Pim));
  uint64_t OpalMask = 1ULL<<BldInfo::CxiDg3Spec;
  uint64_t SpecMask = (1ULL<<BldInfo::FeeSpec0) |
    (1ULL<<BldInfo::SxrSpec0) |
    (1ULL<<BldInfo::XppSpec0);
#define TEST_CREAT(mask, idType, dataType)                  \
  if (im & mask) {                                          \
    Xtc tc(TypeId(TypeId::idType,dataType::Version),        \
           BldInfo(_pid,BldInfo::Type(i)));                 \
    tc.extent += sizeof(dataType);                          \
    dg->insert(tc,_tc_init);                                \
  }
#define TEST_CACHE(mask, idType, dataType)                        \
  if (im & mask) {                                                \
    Xtc tc(idType, BldInfo(_pid,BldInfo::Type(i)));               \
    tc.extent += sizeof(dataType);                                \
    char* p = _cache.fetch(tc);                                   \
    if (p)                                                        \
      dg->insert(tc,p);                                           \
  }

  for(unsigned i=0; i<BldInfo::NumberOf; i++) {
    uint64_t im = 1ULL<<i;
    if (im & _mask) {
      TEST_CREAT(EBeamMask      ,Id_EBeam            ,BldDataEBeam);
      TEST_CREAT(PhaseCavityMask,Id_PhaseCavity      ,BldDataPhaseCavity);
      TEST_CREAT(FEEGasDetMask  ,Id_FEEGasDetEnergy  ,BldDataFEEGasDetEnergy);
      TEST_CREAT(GMDMask        ,Id_GMD              ,BldDataGMD);
      TEST_CACHE(SpecMask       ,_spectrometerType   ,SpectrometerType);
      TEST_CACHE(IpimbMask      ,_ipimbConfigType    ,IpimbConfigType);
      if (im & IpimbMask) {
        Xtc tc(_ipmFexConfigType, BldInfo(_pid,BldInfo::Type(i)));
        tc.extent += sizeof(IpmFexConfigType);
        dg->insert(tc,_tc_init);
      }
      TEST_CACHE(PimMask        ,_tm6740ConfigType   ,TM6740ConfigType);
      TEST_CACHE(PimMask        ,_pimImageConfigType ,PimImageConfigType);
      if (im & PimMask) {
        Xtc tc(_frameFexConfigType, BldInfo(_pid,BldInfo::Type(i)));
        tc.extent += sizeof(FrameFexConfigType);
        dg->insert(tc,&_frameFexConfig);
      }
      TEST_CACHE(OpalMask       ,_opal1kConfigType   ,Opal1kConfigType);
      if (im & OpalMask) {
        Xtc tc(_frameFexConfigType, BldInfo(_pid,BldInfo::Type(i)));
        tc.extent += sizeof(FrameFexConfigType);
        dg->insert(tc,&_frameFexConfig);
      }
      //      TEST_CACHE(AcqMask        ,Id_SharedAcq      ,BldDataAcqADC);
    }
  }
#undef TEST_CACHE
#undef TEST_CREAT
      }
      return dg;
    }
    Transition* transitions(Transition* tr) {
      if (tr->id()==TransitionId::Map) {
        const Allocation& alloc = reinterpret_cast<const Allocate*>(tr)->allocation();
        _mask = alloc.bld_mask();
      }
      return tr;
    }
  public:
#define INSERT(t,v) {                                                   \
      Xtc tc(TypeId(TypeId::Id_##t,v.Version), xtc->src, xtc->damage);  \
      tc.extent += sizeof(v);                                           \
      _dg->insert(tc,&v);                                               \
    }
#define REQUIRE(id)                                                     \
    case TypeId::Id_##id:                                               \
    if (_require(*xtc,*reinterpret_cast<const id##Type*>(xtc->payload()))) \
      _dg->insert(*xtc,xtc->payload());                                 \
        break;

    int process(Xtc* xtc) {
      //  Change the Src process ID to this one
      switch(xtc->src.level()) {
      case Level::Segment:
        { const DetInfo& info = static_cast<const DetInfo&>(xtc->src);
          xtc->src = DetInfo(_pid, info.detector(), info.detId(), info.device(), info.devId());
          break; }
      case Level::Reporter:
        { const BldInfo& info = static_cast<const BldInfo&>(xtc->src);
          if (info.type()==94)  // special exception for broken transmission
            xtc->src = BldInfo(_pid, BldInfo::FeeSpec0);
          else
            xtc->src = BldInfo(_pid, info.type());
          break; }
      default:
        break;
      }
      switch(xtc->contains.id()) {
        //  Iterate through hierarchy
      case TypeId::Id_Xtc: iterate(xtc); break;
        //
        //  Split compound data types
        //
      case TypeId::Id_SharedIpimb:
        if (xtc->contains.version() == BldDataIpimb::Version) {
          const BldDataIpimb* c = reinterpret_cast<const BldDataIpimb*>(xtc->payload());
          if (_require(Xtc(_ipimbConfigType,xtc->src),c->ipimbConfig()))
            INSERT(IpimbConfig,c->ipimbConfig());
          INSERT(IpimbData,c->ipimbData());
          INSERT(IpmFex,c->ipmFexData());
        }
        else
          _abort(xtc->contains);
        break;
      case TypeId::Id_SharedPim:
        if (xtc->contains.version() == BldDataPimV1::Version) {
          const BldDataPimV1* c = reinterpret_cast<const BldDataPimV1*>(xtc->payload());
          if (_require(Xtc(_pimImageConfigType,xtc->src),c->pimConfig()))
            INSERT(PimImageConfig,c->pimConfig());
          if (_require(Xtc(_tm6740ConfigType,xtc->src),c->camConfig()))
            INSERT(TM6740Config,c->camConfig());
          INSERT(Frame,c->frame());
        }
        else
          _abort(xtc->contains);
        break;
#if 0
      case TypeId::Id_SharedAcqADC:
        if (xtc->contains.version() == BldDataAcqADC::Version) {
          const BldDataAcqADC* c = reinterpret_cast<const BldDataAcqADC*>(xtc->payload());
          if (_require(Xtc(_acqConfigType,xtc->src),c->config()))
            INSERT(AcqConfig,c->config());
          INSERT(AcqWaveform,c->data());
        }
        else
          _abort(xtc->contains);
        break;
#endif
        //
        //  Sparsify redundant configuration data
        //
      REQUIRE(AcqConfig   )
      REQUIRE(IpimbConfig )
      REQUIRE(TM6740Config)
      REQUIRE(PimImageConfig)
      REQUIRE(Opal1kConfig)
      REQUIRE(Spectrometer)
      default:
        _dg->insert(*xtc,xtc->payload());
        break;
      }

      return 1;
    }
  private:
    bool _require(const Xtc& xtc, const IpimbConfigType& c) {
      char* p = _cache.fetch(xtc);
      if (!p || memcmp(p,&c,sizeof(c))) {
        _cache.update(xtc,(const char*)&c, sizeof(c));
        if (!p) _reconfigure(xtc);
        return true;
      }
      return false;
    }

    bool _require(const Xtc& xtc, const PimImageConfigType& c) {
      void* p = _cache.fetch(xtc);
      if (!p || memcmp(p,&c,sizeof(c))) {
        _cache.update(xtc, (const char*)&c, sizeof(c));
        if (!p) _reconfigure(xtc);
        return true;
      }
      return false;
    }

    bool _require(const Xtc& xtc,
                  const TM6740ConfigType& c) {
      void* p = _cache.fetch(xtc);
      if (!p || memcmp(p,&c,sizeof(c))) {
        _cache.update(xtc, (const char*)&c, sizeof(c));
        _reconfigure(xtc);
        return true;
      }
      return false;
    }

    bool _require(const Xtc& xtc,
                  const Opal1kConfigType& c) {
      void* p = _cache.fetch(xtc);
      if (!p || memcmp(p,&c,sizeof(c))) {
        _cache.update(xtc, (const char*)&c, sizeof(c));
        _reconfigure(xtc);
        return true;
      }
      return false;
    }

    bool _require(const Xtc& xtc,
                  const SpectrometerType& c) {
      void* p = _cache.fetch(xtc);
      //  Only the first 3 uint32's describe the configuration
      if (!p || memcmp(p,&c,3*sizeof(uint32_t))) {
        _cache.update(xtc, (const char*)&c, sizeof(c));
        _reconfigure(xtc);
      }
      return true;
    }

    bool _require(const Xtc& xtc,
                  const AcqConfigType& c) {
      char* p = _cache.fetch(xtc);
      if (!p || memcmp(p,&c,sizeof(c))) {
        _cache.update(xtc, (const char*)&c, sizeof(c));
        _reconfigure(xtc);
        return true;
      }
      return false;
    }
  private:
    void _reconfigure(const Xtc& xtc) {
      if (!_reconfigure_requested) {
  _reconfigure_requested=true;
  UserMessage* msg = new(&_occ) UserMessage;
  msg->append("BLD Configuration change.  Begin running again.");
  post(msg);
  Occurrence* occ = new(&_occ) Occurrence(OccurrenceId::ClearReadout);
  post(occ);
      }
    }
    void _abort(const TypeId& id) {
      printf("Unknown data version %s.v%d.  Aborting...\n",
       TypeId::name(id.id()),id.version());
      exit(1);
    }
  private:
    GenericPoolW _pool;
    GenericPool _occ;
    BldConfigCache& _cache;
    CDatagram*  _dg;
    uint64_t    _mask;
    bool        _reconfigure_requested;
    unsigned    _pid;
  };

  //
  //  Add temporary configuration objects to indicate which detectors are coming
  //
  class BldConfigApp : public Appliance {
  public:
    BldConfigApp(const Src& src) :
      _configtc       (_xtcType, src),
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
        uint64_t m = alloc.bld_mask();
#define CheckType(bldType)  (m & (1ULL<<BldInfo::bldType))
#define SizeType(dataType)  (sizeof(Xtc) + sizeof(dataType))
#define AddType(bldType,idType,dataType) {        \
    Xtc& xtc = *new(p) Xtc(TypeId(TypeId::idType,(uint32_t)dataType::Version), BldInfo(0,BldInfo::bldType)); \
    xtc.extent = SizeType(dataType);        \
    p += xtc.extent;            \
  }
#define SizeCamType         (sizeof(Xtc)*3 + sizeof(TM6740ConfigType) + sizeof(FrameFexConfigType) + sizeof(PimImageConfigType))
#define AddCamXtc(bldType,idType,dataType) {                            \
    Xtc& xtc = *new(p) Xtc(idType, BldInfo(0,BldInfo::bldType));  \
    xtc.extent += sizeof(dataType);       \
    p += xtc.extent;            \
  }
#define AddCamType(bldType) {                                           \
    AddCamXtc(bldType,_tm6740ConfigType  ,TM6740ConfigType);  \
    AddCamXtc(bldType,_frameFexConfigType,FrameFexConfigType);  \
    AddCamXtc(bldType,_pimImageConfigType,PimImageConfigType);  \
  }
  unsigned extent = 0;
  if (CheckType(EBeam))           extent += SizeType(BldDataEBeam);
  if (CheckType(PhaseCavity))     extent += SizeType(BldDataPhaseCavity);
  if (CheckType(FEEGasDetEnergy)) extent += SizeType(BldDataFEEGasDetEnergy);
  if (CheckType(Nh2Sb1Ipm01))     extent += SizeType(BldDataIpimb);
  if (CheckType(Nh2Sb1Ipm02))     extent += SizeType(BldDataIpimb);
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
  if (CheckType(CxiDg1Imb01))     extent += SizeType(BldDataIpimb);
  if (CheckType(CxiDg2Imb01))     extent += SizeType(BldDataIpimb);
  if (CheckType(CxiDg2Imb02))     extent += SizeType(BldDataIpimb);
  if (CheckType(CxiDg4Imb01))     extent += SizeType(BldDataIpimb);
  if (CheckType(CxiDg1Pim  ))     extent += SizeCamType;
  if (CheckType(CxiDg2Pim  ))     extent += SizeCamType;
  if (CheckType(CxiDg4Pim  ))     extent += SizeCamType;
  if (CheckType(XppMonPim0 ))     extent += SizeType(BldDataIpimb);
  if (CheckType(XppMonPim1 ))     extent += SizeType(BldDataIpimb);
  if (CheckType(XppSb2Ipm  ))     extent += SizeType(BldDataIpimb);
  if (CheckType(XppSb3Ipm  ))     extent += SizeType(BldDataIpimb);
  if (CheckType(XppSb3Pim  ))     extent += SizeType(BldDataIpimb);
  if (CheckType(XppSb4Pim  ))     extent += SizeType(BldDataIpimb);
  if (CheckType(XppEndstation0))  extent += SizeType(BldDataIpimb);
  if (CheckType(XppEndstation1))  extent += SizeType(BldDataIpimb);
  if (CheckType(MecXt2Pim02))     extent += SizeType(BldDataIpimb);
  if (CheckType(MecXt2Pim03))     extent += SizeType(BldDataIpimb);
  if (CheckType(XcsSb1Ipm01))     extent += SizeType(BldDataIpimb);
  if (CheckType(XcsSb1Ipm02))     extent += SizeType(BldDataIpimb);
  if (CheckType(XcsSb2Ipm01))     extent += SizeType(BldDataIpimb);
  if (CheckType(XcsSb2Ipm02))     extent += SizeType(BldDataIpimb);
  if (CheckType(XcsGonIpm01))     extent += SizeType(BldDataIpimb);
  if (CheckType(XcsLamIpm01))     extent += SizeType(BldDataIpimb);

  _configtc.extent = sizeof(Xtc)+extent;
  if (extent) {
    if (_config_payload) delete[] _config_payload;
    char* p = _config_payload = new char[extent];
    if (CheckType(EBeam))           AddType(EBeam,           Id_EBeam,           BldDataEBeam);
    if (CheckType(PhaseCavity))     AddType(PhaseCavity,     Id_PhaseCavity,     BldDataPhaseCavity);
    if (CheckType(FEEGasDetEnergy)) AddType(FEEGasDetEnergy, Id_FEEGasDetEnergy, BldDataFEEGasDetEnergy );
    if (CheckType(Nh2Sb1Ipm01))     AddType(Nh2Sb1Ipm01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(Nh2Sb1Ipm02))     AddType(Nh2Sb1Ipm02,     Id_SharedIpimb,     BldDataIpimb);
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
    if (CheckType(MecTctrPip01))    AddType(MecTctrPip01,    Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(MecTcTrDio01))    AddType(MecTcTrDio01,    Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(MecXt2Ipm02))     AddType(MecXt2Ipm02,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(MecXt2Ipm03))     AddType(MecXt2Ipm03,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(MecHxmIpm01))     AddType(MecHxmIpm01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(GMD))             AddType(GMD,             Id_GMD,             BldDataGMD);
    if (CheckType(CxiDg1Imb01))     AddType(CxiDg1Imb01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(CxiDg2Imb01))     AddType(CxiDg2Imb01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(CxiDg2Imb02))     AddType(CxiDg2Imb02,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(CxiDg4Imb01))     AddType(CxiDg4Imb01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(CxiDg1Pim  ))     AddCamType(CxiDg1Pim);
    if (CheckType(CxiDg2Pim  ))     AddCamType(CxiDg2Pim);
    if (CheckType(CxiDg4Pim  ))     AddCamType(CxiDg4Pim);
    if (CheckType(XppMonPim0 ))     AddType(XppMonPim0,      Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XppMonPim1 ))     AddType(XppMonPim1,      Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XppSb2Ipm  ))     AddType(XppSb2Ipm ,      Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XppSb3Ipm  ))     AddType(XppSb3Ipm ,      Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XppSb3Pim  ))     AddType(XppSb3Pim ,      Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XppSb4Pim  ))     AddType(XppSb4Pim ,      Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XppEndstation0))  AddType(XppEndstation0,  Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XppEndstation1))  AddType(XppEndstation1,  Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(MecXt2Pim02))     AddType(MecXt2Pim02,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(MecXt2Pim03))     AddType(MecXt2Pim03,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XcsSb1Ipm01))     AddType(XcsSb1Ipm01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XcsSb1Ipm02))     AddType(XcsSb1Ipm02,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XcsSb2Ipm01))     AddType(XcsSb2Ipm01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XcsSb2Ipm02))     AddType(XcsSb2Ipm02,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XcsGonIpm01))     AddType(XcsGonIpm01,     Id_SharedIpimb,     BldDataIpimb);
    if (CheckType(XcsLamIpm01))     AddType(XcsLamIpm01,     Id_SharedIpimb,     BldDataIpimb);

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
                uint64_t   mask,
                BldConfigCache& cache,
                bool       lCompress) :
      _task    (task),
      _platform(platform),
      _cache   (cache),
      _lCompress(lCompress)
    {
      Node node(Level::Source,platform);
      _sources.push_back(DetInfo(node.pid(),
         DetInfo::BldEb, 0,
         DetInfo::NoDevice, 0));
      for(unsigned i=0; i<BldInfo::NumberOf; i++)
        if ( (1ULL<<i)&mask )
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
      if (_lCompress)
        (new FrameCompApp(TM6740ConfigType::Row_Pixels*TM6740ConfigType::Column_Pixels*2))->connect(inlet);
      (new BldParseApp(_cache))->connect(inlet);
      //      (new BldConfigApp(_sources.front()))->connect(inlet);
      (new BldDbg(static_cast<EbBase*>(streams.wire())))->connect(inlet);
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
    BldConfigCache& _cache;
    std::list<Src> _sources;
    bool           _lCompress;
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
      EbS(id, ctns, level, inlet, outlet, stream, ipaddress, eventsize, eventpooldepth, 0 /* slow readout = 0 */, vmoneb) {}
    ~BldEvBuilder() {}
  public:
    int processIo(Server* s) { EbS::processIo(s); return 1; }
    int processTmo() { 
      EbBase::processTmo(); 
      arm(managed()); 
      return 1; 
    }
    int poll() {
      if(!ServerManager::poll()) return 0;
      ServerManager::arm(managed());
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

    for(unsigned i=0; i<evrd.numFifoEvents(); i++) {
      const Pds::EvrData::FIFOEvent& fe = evrd.fifoEvents()[i];
      if (fe.timestampHigh() == timestamp &&
          fe.eventCode() >= 140 &&
          fe.eventCode() <= 146)
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

      const Node& node = m.header();
      Level::Type level = node.level();
      int ipaddress = node.ip();
      const Src& src = m.header().procInfo();
      for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

  _outlets[s] = new ToEventWireScheduler(*stream(s)->outlet(),
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
      uint64_t bldmask   = alloc.bld_mask();
      uint64_t bldmaskt  = alloc.bld_mask_mon();
      InletWire & inlet  = *_streams->wire(StreamParams::FrameWork);

      for (unsigned n = 0; n < nnodes; n++) {
        const Node & node = *alloc.node(n);
        if (node.level() == Level::Segment &&
	    node == _header) {
	  _contains = node.transient()?_transientXtcType:_xtcType;  // transitions
	  static_cast<EbBase&>(inlet).contains(_contains);  // l1accepts
	}
      }

      for (unsigned n = 0; n < nnodes; n++) {
        const Node & node = *alloc.node(n);
        if (node.level() == Level::Segment) {
          Ins ins = StreamPorts::event(partition, Level::Observer, 0, 0);
          _evrServer =
            new NullServer(ins,
                           header().procInfo(),
                           sizeof(Pds::EvrData::DataV3)+256*sizeof(Pds::EvrData::FIFOEvent),
                           EvrBufferDepth);

          Ins mcastIns(ins.address());
          _evrServer->server().join(mcastIns, Ins(header().ip()));

          inlet.add_input(_evrServer);
          _inputs.push_back(_evrServer);
          break;
        }
      }

      for(int i=0; i<BldInfo::NumberOf; i++) {
        if (bldmask & (1ULL<<i)) {
          Node node(Level::Reporter, 0);
          node.fixup(StreamPorts::bld(i).address(),Ether());
          Ins ins( node.ip(), StreamPorts::bld(0).portId());
	  BldServer* srv;
	  if (bldmaskt & (1ULL<<i))
	    srv = new BldServerTransient(ins, BldInfo(0,(BldInfo::Type)i), MAX_EVENT_SIZE);
	  else
	    srv = new BldServer(ins, BldInfo(0,(BldInfo::Type)i), MAX_EVENT_SIZE);
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

      //
      //  Assign traffic shaping phase
      //
      const int pid = getpid();
      for(unsigned n=0; n<nnodes; n++) {
        const Node& node = *alloc.node(n);
        if (node.level()==Level::Segment)
          if (node.pid()==pid) {
            ToEventWireScheduler::setPhase  (n % vectorid);
            ToEventWireScheduler::setMaximum(vectorid);
            ToEventWireScheduler::shapeTmo  (alloc.options()&Allocation::ShapeTmo);
          }
      }
    }
    void dissolved() {
      _evrServer = 0;
      for(std::list<Server*>::iterator it=_inputs.begin(); it!=_inputs.end(); it++)
        static_cast <InletWireServer*>(_streams->wire())->remove_input(*it);
      _inputs.clear();

      static_cast <InletWireServer*>(_streams->wire())->remove_outputs();
    }
  private:
#ifdef DBUG
    void post(const Transition& tr) {
      printf("Transition %s\n");
      static_cast<InletWireServer*>(_streams->wire(StreamParams::FrameWork))->dump();

      //      SegmentLevel::post(tr);
      if (tr.id()!=TransitionId::L1Accept) {
        _streams->wire(StreamParams::FrameWork)->flush_inputs();
        _streams->wire(StreamParams::FrameWork)->flush_outputs();
      }
      _streams->wire(StreamParams::FrameWork)->post(tr);
    }
#endif

  private:
    std::list<Server*> _inputs;
  };
}

using namespace Pds;

static BldConfigCache* cache = 0;
static struct sigaction old_actions[64];

static void sigintHandler(int iSignal)
{
  if (cache) cache->store();
  sigaction(iSignal,&old_actions[iSignal],NULL);
  raise(iSignal);
}

void usage(const char* p) {
  printf("Usage: %s -p <platform> [-m <mask>] [-C] [-h]\n\n"
         "Options:\n"
         "       -C : compress images\n"
         "       -h:  print this message and exit\n",p);
}

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = unsigned(-1);
  // parse the command line for our boot parameters
  unsigned platform = NO_PLATFORM;
  bool lCompress = false;
  uint64_t mask = (1ULL<<BldInfo::NumberOf)-1;
  EbBase::printSinks(false);

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "p:m:Ch")) != EOF ) {
    switch(c) {
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'm':
      mask = strtoull(optarg, NULL, 0);
      break;
    case 'C':
      lCompress = true;
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    default:
      usage(argv[0]);
      return 0;
    }
  }

  if (platform==NO_PLATFORM) {
    usage(argv[0]);
    return 0;
  }

  cache = new BldConfigCache;

  // Unix signal support
  struct sigaction int_action;

  int_action.sa_handler = sigintHandler;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;

#define REGISTER(t) {                                   \
  if (sigaction(t, &int_action, &old_actions[t]) > 0)   \
    printf("Couldn't set up #t handler\n"); \
  }

  REGISTER(SIGINT);
  REGISTER(SIGKILL);
  REGISTER(SIGSEGV);
  REGISTER(SIGABRT);
  REGISTER(SIGTERM);

  Task*               task = new Task(Task::MakeThisATask);
  BldCallback*          cb = new BldCallback(task, platform, mask, *cache, lCompress);
  BldSegmentLevel* segment = new BldSegmentLevel(platform, *cb);
  if (segment->attach())
    task->mainLoop();

  segment->detach();
  return 0;
}
