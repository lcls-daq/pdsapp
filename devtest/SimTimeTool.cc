#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/config/CfgCache.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TimeToolConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <math.h>

//#define DBUG

namespace Pds {
  class TimeToolCfgCache : public CfgCache {
  public:
    TimeToolCfgCache(const Src& src, 
		     const TypeId& id,
		     int size) :
      CfgCache(src,id,size) {}
  public:
    int _size(void* p) const { return reinterpret_cast<TimeToolConfigType*>(p)->_sizeof(); }
  };

  class ConfigHandler : public XtcIterator {
  public:
    ConfigHandler() : 
      _cache      (0),
      _allocation (0),
      _configure  (0),
      _src(Level::Control) {}
  public:
    int process(Xtc* xtc) {
      switch(xtc->contains.id()) {
      case TypeId::Id_Xtc:
	iterate(xtc);
	break;
      case TypeId::Id_Opal1kConfig:
	{ TimeToolConfigType tmplate(4,4,256,8,64);
	  _cache = new TimeToolCfgCache(_src = xtc->src,
					_timetoolConfigType,
					tmplate._sizeof());
	  _cache->init(*_allocation);
	  int s = _cache->fetch(_configure);
	  printf("SimTimeTool::Config::fetch [%s]=%d\n",
		 DetInfo::name(static_cast<DetInfo&>(xtc->src)),s);
	} break;
      default:
	break;
      }
      return 1;
    }
  public:
    void transitions(Transition* tr) {
      switch (tr->id()) {
      case TransitionId::Configure:
	_configure = new Transition(tr->id(), 
				    tr->phase(),
				    tr->sequence(),
				    tr->env());
	break;
      case TransitionId::Unconfigure:
	delete _configure;
	break;
      case TransitionId::Map:
	{ const Allocation& alloc = reinterpret_cast<const Allocate*>(tr)->allocation();
	  _allocation = new Allocation(alloc);
	} break;
      case TransitionId::Unmap:
	delete _allocation;
	break;
      default:
	break;
      }
    }
    void events(InDatagram* dg) {
      if (dg->seq.service()!=TransitionId::Configure)
	return;

      if (_cache) {
	delete _cache;
	_cache = 0;
      }

      process(&dg->xtc);
    }
    const TimeToolConfigType* cache() const { return _cache && _cache->current() ? 
	reinterpret_cast<const TimeToolConfigType*>(_cache->current()) : 0; }
    const DetInfo& src() const { return static_cast<const DetInfo&>(_src); }
  private:
    TimeToolCfgCache* _cache;
    const Allocation* _allocation;
    const Transition* _configure;
    Src               _src;
  };

  class SimTimeTool : public Appliance,
		      public XtcIterator {
  public:
    SimTimeTool();
  public:
    Transition* transitions(Transition* tr);
    InDatagram* events     (InDatagram* dg);
    Occurrence* occurrences(Occurrence* occ);
  public:
    int process(Xtc*);
  private:
    ConfigHandler       _configH;
    enum { NBuffers=16, SeedMask=NBuffers-1 };
    ndarray<uint16_t,3> _laseroff;
    ndarray<uint16_t,3> _beamoff;
    ndarray<uint16_t,3> _beamon;
    ndarray<uint32_t,1> _beam;
    ndarray<uint32_t,1> _laser;
    unsigned _vector;
  };
};

using namespace Pds;

static void _set_dark(ndarray<uint16_t,3> a, unsigned offset)
{
  const unsigned NR=11;
  uint16_t r[NR];
  for(unsigned i=0; i<NR; i++)
    r[i] = rand()>>2;

  uint16_t* p = a.data();
  for(unsigned i=0; i<a.size(); i++)
    *p++ = (r[i%NR]&0xf) + offset - 0x8;
}

static void _add_laser(ndarray<uint16_t,3> a,
		       double tloss)
{
  static const double AMPL = 100;
  static const double OMEGA = M_PI/200.;

  ndarray<double,1> vx = make_ndarray<double>(a.shape()[2]);
  for(unsigned i=0; i<vx.size(); i++)
    vx[i] = (1-cos(OMEGA*double(i)))+0.2;

  for(unsigned i=0; i<a.shape()[0]; i++) {
    unsigned edge = (rand()>>4)%(a.shape()[2]);
    printf("edge[%u] = %u\n",i,edge);
    for(unsigned iy=200; iy<400; iy++) {
      uint16_t* p=&a[i][iy][0];
      double ampl = AMPL*(1-tloss);
      for(unsigned ix=0; ix<edge; ix++)
	*p++ += int(ampl*vx[ix]);
      for(unsigned ix=edge; ix<a.shape()[2]; ix++)
	*p++ += int(AMPL*vx[ix]);
    }
    for(unsigned iy=800; iy<1000; iy++) {
      uint16_t* p=&a[i][iy][0];
      for(unsigned ix=0; ix<a.shape()[2]; ix++)
	*p++ += int(AMPL*vx[ix]);
    }
  }
}

static bool _calculate_logic(const ndarray<const TimeTool::EventLogic,1>& cfg,
			     const ndarray<uint32_t,1>& value,
			     uint32_t vector)
{
  bool v = (cfg[0].logic_op() == TimeTool::EventLogic::L_AND ||
	    cfg[0].logic_op() == TimeTool::EventLogic::L_AND_NOT);
  for(unsigned i=0; i<cfg.size(); i++)
    switch(cfg[i].logic_op()) {
    case TimeTool::EventLogic::L_OR:
      v = v||(value[i]&vector); break;
    case TimeTool::EventLogic::L_AND:
      v = v&&(value[i]&vector); break;
    case TimeTool::EventLogic::L_OR_NOT:
      v = v||!(value[i]&vector); break;
    case TimeTool::EventLogic::L_AND_NOT:
      v = v&&!(value[i]&vector); break;
    default: break;
    }
  return v;
}

SimTimeTool::SimTimeTool()
{
}

Transition* SimTimeTool::transitions(Transition* tr)
{
  _configH.transitions(tr);
  return tr;
}

InDatagram* SimTimeTool::events(InDatagram* dg)
{
  _configH.events(dg);

  _vector = 1<<(dg->seq.stamp().vector()&0x1f);

  switch(dg->datagram().seq.service()) {
  case TransitionId::Configure:
    { unsigned rows = Opal1k::max_row_pixels(_configH.src());
      unsigned cols = Opal1k::max_column_pixels(_configH.src());
      printf("Constructing arrays of size %d,%d,%d\n",
             NBuffers,rows,cols);
      _laseroff = make_ndarray<uint16_t>(NBuffers,rows,cols);
      _set_dark(_laseroff, 32);

      _beamoff = make_ndarray<uint16_t>(NBuffers,rows,cols);
      _set_dark(_beamoff, 32);
      _add_laser(_beamoff, 0.);

      _beamon = make_ndarray<uint16_t>(NBuffers,rows,cols);
      _set_dark(_beamon, 32);
      _add_laser(_beamon, 0.2);
    } break;
  case TransitionId::L1Accept:
    iterate(&dg->datagram().xtc);
    break;
  default:
    break;
  }

  const TimeToolConfigType* cfg = _configH.cache();
  if (cfg) {
    switch (dg->seq.service()) {
    case TransitionId::Configure:
      _beam  = make_ndarray<uint32_t>(cfg->number_of_beam_event_codes());
      memset(_beam.data(), 0, _beam.size()*sizeof(uint32_t));
      _laser = make_ndarray<uint32_t>(cfg->number_of_laser_event_codes());
      memset(_laser.data(), 0, _laser.size()*sizeof(uint32_t));
      break;
    case TransitionId::L1Accept:
      for(unsigned i=0; i<_beam.size(); i++)
	_beam[i] &= ~_vector;
      for(unsigned i=0; i<_laser.size(); i++)
	_laser[i] &= ~_vector;
      break;
    default:
      break;
    }
  }

  return dg;
}

Occurrence* SimTimeTool::occurrences(Occurrence* occ)
{
  if (occ->id() == OccurrenceId::EvrCommand) {
    const EvrCommand& cmd = *reinterpret_cast<const EvrCommand*>(occ);
    unsigned vector = 1<<(cmd.seq.stamp().vector()&0x1f);
    const TimeToolConfigType* cfg = _configH.cache();
    if (cfg) {
      ndarray<const TimeTool::EventLogic,1> beam = cfg->beam_logic();
      for(unsigned i=0; i<beam.size(); i++)
	if (beam[i].event_code() == cmd.code)
	  _beam[i] |= vector;

      ndarray<const TimeTool::EventLogic,1> laser = cfg->laser_logic();
      for(unsigned i=0; i<laser.size(); i++)
	if (laser[i].event_code() == cmd.code)
	  _laser[i] |= vector;
    }
  }
  return occ;
}

int SimTimeTool::process(Xtc* xtc)
{
  switch(xtc->contains.id()) {
  case TypeId::Id_Xtc:
    iterate(xtc);
    break;
  case TypeId::Id_Frame:
    { const TimeToolConfigType* cfg = _configH.cache();
      if (cfg) {
	bool beamOn  = _calculate_logic(cfg->beam_logic(),
					_beam, _vector);

	bool laserOn = _calculate_logic(cfg->laser_logic(),
					_laser, _vector);

#ifdef DBUG
	printf("vector %08x  beam %c  laser %c\n", _vector, beamOn ? 'T':'F', laserOn ? 'T':'F');
#endif

	Camera::FrameV1& d = *reinterpret_cast<Camera::FrameV1*>(xtc->payload());
	unsigned seed = rand()%NBuffers;
	if (!laserOn)
	  memcpy(const_cast<uint16_t*>(d.data16().data()),
		 &_laseroff[seed][0][0],
		 d.data16().size()*sizeof(uint16_t));
	else if (!beamOn)
	  memcpy(const_cast<uint16_t*>(d.data16().data()),
		 &_beamoff[seed][0][0],
		 d.data16().size()*sizeof(uint16_t));
	else
	  memcpy(const_cast<uint16_t*>(d.data16().data()),
		 &_beamon[seed][0][0],
		 d.data16().size()*sizeof(uint16_t));
      }
    }
    break;
  default:
    break;
  }
  return 1;
}

//
//  Plug-in module creator
//
extern "C" Appliance* create() { return new SimTimeTool; }

extern "C" void destroy(Appliance* p) { delete p; }
