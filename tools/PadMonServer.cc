#include "PadMonServer.hh"

#include "pdsapp/tools/CspadShuffle.hh"

#include "pdsdata/app/XtcMonitorServer.hh"

#include "pdsdata/cspad/ConfigV3.hh"
#include "pdsdata/cspad/ElementV1.hh"

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
                                    DetInfo(0,DetInfo::CxiEndstation,0,DetInfo::Fexamp  ,0) };

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
                                TimeStamp(0,0,0,0));
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

  new((void*)&dg->seq) Sequence(Sequence::Event, 
                                tr, 
                                ClockTime(tv.tv_sec,tv.tv_nsec), 
                                TimeStamp(0,0,0,0));
  Xtc* seg = new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), segInfo);
  Xtc* src = new((char*)seg->alloc(sizeofPayload+sizeof(Xtc))) Xtc(tid, srcid);
  char* p  = new((char*)src->alloc(sizeofPayload)) char[sizeofPayload];
  memcpy(p,payload,sizeofPayload);
  return dg;
}

static const unsigned sizeofBuffers = 0xA00000;
static const unsigned numberofBuffers = 2;
static const unsigned nclients = 2;
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

void PadMonServer::configure(const Pds::CsPad::ConfigV3& c)
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

void PadMonServer::unconfigure()
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Disable));
  _srv->events(insert(_srv->newDatagram(), TransitionId::EndCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::EndRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Unconfigure));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Unmap));
}
