#include "pdsapp/tools/JungfrauSegBuilder.hh"

#include "pds/jungfrau/Segment.hh"
#include "pds/config/JungfrauConfigType.hh"

#include "pds/client/XtcStripper.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/xtc/Datagram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"

#include <stdio.h>
#include <set>

//#define DBUG

using namespace Pds;
using namespace Pds::Jungfrau;

static const int _minimum_segments = 2;
static uint32_t _config_extent;
static uint32_t _data_extent;
static std::set<Pds::DetInfo> _info;
static std::vector<JungfrauConfigType> _config;
static SegmentMap _segment_config;
static MultiSegmentMap _config_parts;
static FrameCacheMap _frames;
static ParentMap _parents;

class JungfrauBuilder : public XtcStripper {
public:
  enum Status {Stop, Continue};
  JungfrauBuilder(Xtc* root, uint32_t*& pwrite) : 
    XtcStripper(root,pwrite),
    _fatalError(false) {}
  ~JungfrauBuilder() {}
  bool fatalError() const { return _fatalError; }

protected:
  void process(Xtc* xtc) {
    const DetInfo& info = *(DetInfo*)(&xtc->src);
    switch (xtc->contains.id()) {
    case (TypeId::Id_Xtc) :
      {
        JungfrauBuilder iter(xtc,_pwrite);
        iter.iterate();
      }
      break;
    case (TypeId::Id_JungfrauConfig) :
      switch (info.device()) {
      case (DetInfo::JungfrauSegment) :
        {
          const JungfrauConfigType* cfg = reinterpret_cast<const JungfrauConfigType*>(xtc->payload());
           SegmentConfig* seg_cfg = new SegmentConfig(info, xtc->damage, *cfg);
          _info.insert(seg_cfg->parent());
          _parents.insert(std::make_pair(info, seg_cfg->parent()));
          _segment_config.insert(std::make_pair(info, seg_cfg));
          _config_parts.insert(std::make_pair(seg_cfg->parent(), seg_cfg));
        }
        break;
      default:
        XtcStripper::process(xtc);
        break;
      }
      break;
    case (TypeId::Id_JungfrauElement) :
      switch (info.device()) {
      case (DetInfo::JungfrauSegment) :
        {
          const JungfrauDataType* data = reinterpret_cast<const JungfrauDataType*>(xtc->payload());
          ParentIter it = _parents.find(info);
          SegmentIter cfg_it = _segment_config.find(info);
          if (it != _parents.end() && cfg_it !=_segment_config.end()) {
            FrameCacheIter fc_it = _frames.find(it->second);
            if (fc_it != _frames.end()) {
              fc_it->second->insert(cfg_it->second, data, xtc->damage);
            } else {
              printf("%s. No FrameCache found for parent: %s\n", DetInfo::name(info), DetInfo::name(it->second));
              _fatalError = true;
            }
          } else {
            printf("%s. Found unexpected JungfrauSegment!\n", DetInfo::name(info));
            _fatalError = true;
          }
        }
        break;
      default:
        XtcStripper::process(xtc);
        break;
      }
      break;
    default:
      XtcStripper::process(xtc);
      break;
    }
  }

private:
  bool  _fatalError;
};

static bool process_config(const DetInfo& parent, Xtc* xtc)
{
  unsigned module_mask = 0;
  unsigned num_modules = 0;
  JungfrauConfigType* tmp = NULL;
  JungfrauModConfigType modcfg[JungfrauConfigType::MaxModulesPerDetector];
  MultiSegmentRange range = _config_parts.equal_range(parent);
  for (MultiSegmentIter it = range.first; it != range.second; ++it) {
    const JungfrauConfigType& cfg = it->second->config();
    unsigned expected = it->second->detSize();
    unsigned index = it->second->index();
    unsigned mask = (((1U << (index + cfg.numberOfModules())) - 1) ^ ((1U << (index)) - 1));
    if ((index+cfg.numberOfModules()) > JungfrauConfigType::MaxModulesPerDetector) {
      printf("%s. Module number from segments %u exceeds maximum of %u\n",
             DetInfo::name(parent), index+cfg.numberOfModules(), JungfrauConfigType::MaxModulesPerDetector);
      return false;
    }
    if (module_mask & mask) {
      printf("%s. Duplicate modules from the segments: 0x%04x\n", DetInfo::name(parent), module_mask & mask);
      return false;
    }
    module_mask |= mask;
    DamageHelper::merge(xtc->damage, it->second->damage());
    if (tmp) {
      if (expected != num_modules) {
        printf("%s. Inconsistent expected module number from the segments: %u vs %u\n",
               DetInfo::name(parent), num_modules, expected);
        return false;
      } else if (it->second->verify(*tmp)) {
        printf("%s. Inconsistent configuration from the segments\n", DetInfo::name(parent));
        return false;
      }
    } else {
      num_modules = expected;
      if (num_modules > JungfrauConfigType::MaxModulesPerDetector) {
        printf("%s. Detector has more modules than the maximum supported: %u vs %u\n",
               DetInfo::name(parent), num_modules, JungfrauConfigType::MaxModulesPerDetector);
        return false;
      } else if (num_modules == 0) {
        printf("%s. Detector has no modules!\n",
               DetInfo::name(parent));
        return false;
      }
      tmp = new (xtc->alloc(sizeof(JungfrauConfigType))) JungfrauConfigType(cfg);
    }
    for (unsigned n=0; n<cfg.numberOfModules(); n++) {
      modcfg[index+n] = cfg.moduleConfig(n);
    }
  }
  if (module_mask != ((1U<<num_modules) - 1)) {
    printf("%s. Inconsistent module mask from the segments: 0x%04x vs 0x%04x\n",
           DetInfo::name(parent), module_mask, (1U<<num_modules) - 1);
    return false;
  } else {
    JungfrauConfig::setSize(*tmp,
                            num_modules,
                            tmp->numberOfRowsPerModule(),
                            tmp->numberOfColumnsPerModule(),
                            modcfg);
    _config.push_back(*tmp);
    _frames.insert(std::make_pair(parent, new FrameCache(*tmp)));
  }

  return true;
}

static bool process_data(const FrameCache* fc, Xtc* xtc)
{
  if (fc) {
    memcpy(xtc->alloc(fc->size()), fc->frame(), fc->size());
    return true;
  } else {
    return false;
  }
}

static uint32_t get_config_extent()
{
  return sizeof(Xtc) + (sizeof(Xtc) + sizeof(JungfrauConfigType)) * _info.size();
}

static uint32_t get_data_extent()
{
  uint32_t size = sizeof(Xtc);
  for(FrameCacheIter it=_frames.begin(); it!=_frames.end(); ++it) {
    size += sizeof(Xtc) + it->second->size();
  }
  return size;
}

static bool check_config()
{
  return !_info.empty();
}

static bool check_data()
{
  for(FrameCacheIter it=_frames.begin(); it!=_frames.end(); ++it) {
    if (it->second->ready()) return true;
  }

  return false;
}

bool JungfrauSegBuilder::build(Dgram& dg)
{
  ProcInfo proc(Pds::Level::Segment,0,0);
  return process(dg, proc);
}

bool JungfrauSegBuilder::build(Dgram& dg, const ProcInfo& proc)
{
  ProcInfo seg_proc(Pds::Level::Segment, proc.processId(), proc.ipAddr());
  return process(dg, seg_proc);
}

bool JungfrauSegBuilder::process(Dgram& dg, const ProcInfo& proc) 
{
  bool failed = false;

  if (dg.seq.service() == TransitionId::Configure) {
    // clear up the configs from previous transitions
    _config_extent = 0;
    _data_extent = 0;
    _config.clear();
    _config_parts.clear();
    _info.clear();
    _parents.clear();
    for(SegmentIter it=_segment_config.begin(); it!=_segment_config.end(); ++it)
      delete it->second;
    _segment_config.clear();
    for(FrameCacheIter it=_frames.begin(); it!=_frames.end(); ++it)
      delete it->second;
    _frames.clear();

    // modify the datagram removing segment configs
    uint32_t remaining = dg.xtc.extent;
    uint32_t* pdg = reinterpret_cast<uint32_t*>(&(dg.xtc));
    JungfrauBuilder iter(&(dg.xtc),pdg);
    iter.iterate();
    remaining -= dg.xtc.extent;

    // Calculate the space needed for the build config and data xtcs
    _config_extent = get_config_extent();
    _data_extent = get_data_extent();

    // Create a new segment level entry and add combined Jungfrau configs to it
    if (check_config()) {
      // Check that we have enough space in the datagram for configs we plan to add
      if (_config_extent <= remaining) {
        Xtc* seg = new (&dg.xtc) Xtc(_xtcType, proc);
        for (std::set<Pds::DetInfo>::const_iterator parent=_info.begin(); parent!=_info.end(); ++parent) {
          Xtc* xtc = new (seg) Xtc(_jungfrauConfigType, *parent);
          if (!process_config(*parent, xtc)) {
            printf("%s. Failed to create configuration!\n", DetInfo::name(*parent));
            failed = true;
          }
          seg->alloc(xtc->sizeofPayload());
        }
        dg.xtc.alloc(seg->sizeofPayload());
      } else {
        printf("Not enough space to create configurations, since %u is needed but only %u available!\n", _config_extent, remaining);
        failed = true;
      }
    }
  } else if (!_config.empty() && dg.seq.service() == TransitionId::L1Accept) {
    // modify the datagram removing segment configs
    uint32_t remaining = dg.xtc.extent;
    uint32_t* pdg = reinterpret_cast<uint32_t*>(&(dg.xtc));
    JungfrauBuilder iter(&(dg.xtc),pdg);
    iter.iterate();
    remaining -= dg.xtc.extent;

    if (iter.fatalError()) {
      failed = true;
    } else {
      // Create a new segment level entry and add combined Jungfrau frames to it
      if (check_data()) {
        // Check that we have enough space in the datagram for data we plan to add
        if (_data_extent <= remaining) {
          Xtc* seg = new (&dg.xtc) Xtc(_xtcType, proc);
          for (FrameCacheIter it=_frames.begin(); it!=_frames.end(); ++it) {
            Xtc* xtc = new (seg) Xtc(_jungfrauDataType, it->first);
            if (!process_data(it->second, xtc)) {
              printf("%s. Failed to create data!\n", DetInfo::name(it->first));
              failed = true;
            }
            seg->alloc(xtc->sizeofPayload());
            // reset the frame cache state
            it->second->reset();
          }
          dg.xtc.alloc(seg->sizeofPayload());
        } else {
          printf("Not enough space to add data, since %u is needed but only %u available!\n", _config_extent, remaining);
          failed = true;
        }
      } else {
        for (FrameCacheIter it=_frames.begin(); it!=_frames.end(); ++it)
          it->second->reset();
      }
    }
  }

  return !failed;
}
