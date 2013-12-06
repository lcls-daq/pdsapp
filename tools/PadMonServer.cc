#include "PadMonServer.hh"

#include "pdsapp/tools/CspadShuffle.hh"

#include "pds/config/ImpConfigType.hh"

#include "pdsdata/app/XtcMonitorServer.hh"

#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/psddl/imp.ddl.h"
#include "pdsdata/psddl/opal1k.ddl.h"
#include "pdsdata/psddl/camera.ddl.h"
//#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/psddl/epixsampler.ddl.h"

#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/Dgram.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <queue>

static Pds::TypeId _testCnfgType(Pds::TypeId::Any,1);
static Pds::TypeId _testDataType(Pds::TypeId::Any,2);
static unsigned    _ievent=0;

namespace Pds {

  class MyMonitorServer : public XtcMonitorServer {
  public:
    MyMonitorServer(const char* tag,
                    unsigned sizeofBuffers, 
                    unsigned numberofEvBuffers, 
                    unsigned numberofClients,
                    unsigned sequenceLength) :
      XtcMonitorServer(tag,
                       sizeofBuffers,
                       numberofEvBuffers,
                       numberofClients,
                       sequenceLength) 
    {
      //  sum of client queues (nEvBuffers) + clients + transitions + shuffleQ
      unsigned depth = 2*numberofEvBuffers+XtcMonitorServer::numberofTrBuffers+numberofClients;
      for(unsigned i=0; i<depth; i++)
        _pool.push(reinterpret_cast<Dgram*>(new char[sizeofBuffers]));
    }
    ~MyMonitorServer() 
    {
      while(!_pool.empty()) {
        delete _pool.front();
        _pool.pop();
      }
    }
  public:
    XtcMonitorServer::Result events(Dgram* dg) {
      if (XtcMonitorServer::events(dg) == XtcMonitorServer::Handled)
        _deleteDatagram(dg);
      return XtcMonitorServer::Deferred;
    }
    Dgram* newDatagram() 
    { 
      Dgram* dg = _pool.front(); 
      _pool.pop(); 
      return dg; 
    }
    void   deleteDatagram(Dgram* dg) { _deleteDatagram(dg); }
  private:
    void  _deleteDatagram(Dgram* dg)
    {
      _pool.push(dg); 
    }
  private:
    std::queue<Dgram*> _pool;
  };
};

using namespace Pds;

static const ProcInfo segInfo(Pds::Level::Segment,0,0);
static const DetInfo  srcInfo[] = { DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::Cspad   ,0),
                                    DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::Cspad2x2,0),
                                    DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::Fexamp  ,0),
                                    DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::Imp     ,0),
                                    DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::Epix    ,0),
                                    DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::EpixSampler,0),
				    DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::NumDevice,0)};

//
//  Insert a simulated transition
//
static Dgram* insert(Dgram*              dg,
                     TransitionId::Value tr)
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);

  new((void*)&dg->seq) Sequence(Sequence::Event, 
                                tr, 
                                ClockTime(tv.tv_sec,tv.tv_nsec), 
                                TimeStamp(0,0x1ffff,0,0));
  new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), segInfo);
  return dg;
}

static Dgram* insert(Dgram*              dg,
                     TransitionId::Value tr, 
                     TypeId              tid,
                     const Src&          srcid,
                     const void*         payload, 
                     unsigned            sizeofPayload)
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);

  unsigned v = tr==TransitionId::L1Accept ? _ievent++ : 0;
  new((void*)&dg->seq) Sequence(Sequence::Event, 
                                tr, 
                                ClockTime(tv.tv_sec,tv.tv_nsec), 
                                TimeStamp(0,0x1ffff,v,0));
  Xtc* seg = new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), segInfo);
  Xtc* src = new((char*)seg->alloc(sizeofPayload+sizeof(Xtc))) Xtc(tid, srcid);
  char* p  = new((char*)src->alloc(sizeofPayload)) char[sizeofPayload];
  memcpy(p,payload,sizeofPayload);
  return dg;
}

static const unsigned sizeofBuffers = 0xA00000;
static const unsigned numberofBuffers = 8;
static const unsigned nclients = 1;
static const unsigned sequenceLength = 1;
static unsigned payloadsize = 0;

PadMonServer::PadMonServer(PadType t,
                           const char* partitionTag) :
  _t   (t),
  _srv (new MyMonitorServer(partitionTag,
                            sizeofBuffers, 
                            numberofBuffers, 
                            nclients,
                            sequenceLength))
{
}

PadMonServer::~PadMonServer()
{
  delete _srv;
}

void PadMonServer::configure(const Pds::CsPad::ConfigV5& c)
{
  unsigned nq = 0;
  unsigned qmask = c.quadMask();
  for(unsigned i=0; i<4; i++)
    if (qmask & (1<<i))
      nq++;

  payloadsize = nq*c.payloadSize();

  _srv->events(insert(_srv->newDatagram(), TransitionId::Map));

  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::Configure, 
         TypeId(TypeId::Id_CspadConfig,3),
         srcInfo[_t],
         &c, 
         sizeof(c));
  CspadShuffle::shuffle(*dg);
  _srv->events(dg);

  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Enable));
}

void PadMonServer::event    (const Pds::CsPad::MiniElementV1& e)
{
  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::L1Accept, 
         TypeId(TypeId::Id_Cspad2x2Element,1),
         srcInfo[_t],
         &e, 
         payloadsize);
  CspadShuffle::shuffle(*dg);
  _srv->events(dg);
}

void PadMonServer::event    (const Pds::CsPad::ElementV1& e)
{
  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::L1Accept, 
         TypeId(TypeId::Id_CspadElement,1),
         srcInfo[_t],
         &e, 
         payloadsize);
  CspadShuffle::shuffle(*dg);
  _srv->events(dg);
}

void PadMonServer::configure(const Pds::Fexamp::ConfigV1&)
{
}

void PadMonServer::event    (const Pds::Fexamp::ElementV1&)
{
}

void PadMonServer::configure(const Pds::Imp::ConfigV1& c)
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Map));

  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::Configure, 
         TypeId(TypeId::Id_ImpConfig,1),
         srcInfo[_t],
         &c, 
         sizeof(c));
  payloadsize = Pds::Imp::ElementV1::_sizeof(c);
  _srv->events(dg);

  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Enable));
}

void PadMonServer::event    (const Pds::Imp::ElementV1& e)
{
  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::L1Accept, 
         TypeId(TypeId::Id_ImpData,1),
         srcInfo[_t],
         &e, 
         payloadsize);
  _srv->events(dg);
}

void PadMonServer::configure(const Pds::Epix::ConfigV1& c)
{
#if 0
  _srv->events(insert(_srv->newDatagram(), TransitionId::Map));

  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::Configure, 
         TypeId(TypeId::Id_EpixConfig,1),
         srcInfo[_t],
         &c, 
         c._sizeof());
  payloadsize = Pds::Epix::ElementV1::_sizeof(c);
  _srv->events(dg);

  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Enable));
#endif
}

void PadMonServer::event    (const Pds::Epix::ElementV1& e)
{
#if 0
  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::L1Accept, 
         TypeId(TypeId::Id_EpixElement,1),
         srcInfo[_t],
         &e, 
         payloadsize);
  _srv->events(dg);
#endif
}


void PadMonServer::configure(const Pds::EpixSampler::ConfigV1& c)
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Map));

  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::Configure, 
         TypeId(TypeId::Id_EpixSamplerConfig,1),
         srcInfo[_t],
         &c, 
         c._sizeof());
  payloadsize = Pds::EpixSampler::ElementV1::_sizeof(c);
  _srv->events(dg);

  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Enable));
}

void PadMonServer::event    (const Pds::EpixSampler::ElementV1& e)
{
  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::L1Accept, 
         TypeId(TypeId::Id_EpixSamplerElement,1),
         srcInfo[_t],
         &e, 
         payloadsize);
  _srv->events(dg);
}


void PadMonServer::config_1d(unsigned nsamples)
{
  Pds::Imp::ConfigV1 c(0, 0, 0, 0, 0, 0, 0, nsamples, 0, 0);
  configure(c);
}

void PadMonServer::event_1d (const uint16_t* d,
			     unsigned        nstep) 
{
  char* p = new char[payloadsize];
  Pds::Imp::ElementV1* e = new(p) Pds::Imp::ElementV1;
  uint16_t* v = reinterpret_cast<uint16_t*>(e+1);
  uint16_t* end = reinterpret_cast<uint16_t*>(p+payloadsize);
  while(v < end) {
    v[0] = *d;
    v[1] = v[2] = v[3] = 0;
    v += Pds::Imp::Sample::channelsPerDevice;
    d += nstep;
  }

  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::L1Accept, 
         TypeId(TypeId::Id_ImpData,1),
         srcInfo[_t],
         p, 
         payloadsize);
  _srv->events(dg);
}

void PadMonServer::config_2d(unsigned nasics_x, unsigned nasics_y, unsigned nsamples,
			     unsigned asic_x, unsigned asic_y)
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Map));

  uint32_t config[5];
  /*
  **  Prototype layout is 1 ADC(chip) x 1 sample(time) x [96x96] channels(pixels)
  */
  config[0] = nasics_x;
  config[1] = nasics_y;
  config[2] = nsamples;
  config[3] = asic_x;
  config[4] = asic_y;
  payloadsize = 8*sizeof(uint32_t) + ((nasics_x*nasics_y+1)&~1)*asic_x*asic_y*nsamples*sizeof(uint16_t);

  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::Configure, 
	 _testCnfgType,
         srcInfo[_t],
         &config, 
         5*sizeof(uint32_t));
  _srv->events(dg);

  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Enable));
}

void PadMonServer::event_2d (const uint16_t* d)
{
  Dgram* dg = _srv->newDatagram();
  insert(dg,
         TransitionId::L1Accept, 
	 _testDataType,
         srcInfo[_t],
         d, 
         payloadsize);
  _srv->events(dg);
}

void PadMonServer::unconfigure()
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Disable));
  _srv->events(insert(_srv->newDatagram(), TransitionId::EndCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::EndRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Unconfigure));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Unmap));
}
