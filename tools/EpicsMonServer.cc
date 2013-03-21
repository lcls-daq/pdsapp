#include "EpicsMonServer.hh"

#include "pdsdata/app/XtcMonitorServer.hh"

#include "pdsdata/epics/EpicsPvData.hh"
#include "pdsdata/epics/EpicsDbrTools.hh"

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
#include <new>

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
#if 0
      {
        printf("event %p\n",dg);
        unsigned* p = reinterpret_cast<unsigned*>(dg);
        for(unsigned i=0; i<sizeof(Dgram)+dg->xtc.sizeofPayload()>>2; i++)
          printf("%08x%c", p[i], (i%8)==7 ? '\n' : ' ');
        printf("\n");
      }
#endif
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
static const DetInfo  srcInfo(0,DetInfo::EpicsArch,0,DetInfo::NoDevice,0);

//
//  Insert a simulated transition
//
static Dgram* insert(Dgram*              dg,
                     TransitionId::Value tr)
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);

  unsigned fiducials = (dg - (Dgram*)0)&0x1ffff;
  new((void*)&dg->seq) Sequence(Sequence::Event, 
                                tr, 
                                ClockTime(tv.tv_sec,tv.tv_nsec), 
                                TimeStamp(0,fiducials,0,0));
  new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), segInfo);
  return dg;
}

static const unsigned sizeofBuffers = 0x10000;
static const unsigned numberofBuffers = 2;
static const unsigned nclients = 2;
static const unsigned sequenceLength = 1;

EpicsMonServer::EpicsMonServer(const char* partitionTag) :
  _srv (new MyMonitorServer(partitionTag,
                            sizeofBuffers, 
                            numberofBuffers, 
                            nclients,
                            sequenceLength))
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Map));
}

EpicsMonServer::~EpicsMonServer()
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Unmap));
  delete _srv;
}

void EpicsMonServer::configure(const std::vector<std::string>& c)
{
  _names = c;

  Dgram* dg = _srv->newDatagram();

  Xtc& segxtc = insert(dg, TransitionId::Configure)->xtc;

  for(unsigned i=0; i<c.size(); i++) {
    Xtc* xtc = new (&segxtc) Xtc(TypeId(TypeId::Id_Epics,1), srcInfo);
    new ((char*)xtc->alloc((sizeof(Pds::EpicsPvCtrlHeader)+3)&~3)) Pds::EpicsPvCtrlHeader(i, DBR_CTRL_DOUBLE, 1, c[i].c_str());
    segxtc.extent += xtc->sizeofPayload();
  }
                           
  _srv->events(dg);

  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::BeginCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Enable));
}

void EpicsMonServer::event    (const std::vector<double>& v)
{
  Dgram* dg = _srv->newDatagram();
  Xtc& segxtc = insert(dg, TransitionId::L1Accept)->xtc;

  struct Pds::Epics::dbr_time_double p;
  memset(&p,0,sizeof(p));
  for(unsigned i=0; i<v.size(); i++) {
    Xtc* xtc = new (&segxtc) Xtc(TypeId(TypeId::Id_Epics,1), srcInfo);
    p.value = v[i];
    new ((char*)xtc->alloc((sizeof(Pds::EpicsPvTime<DBR_DOUBLE>)+3)&~3)) Pds::EpicsPvTime<DBR_DOUBLE>(i, 1, &p);
    segxtc.extent += xtc->sizeofPayload();
  }

  _srv->events(dg);
}

void EpicsMonServer::unconfigure()
{
  _srv->events(insert(_srv->newDatagram(), TransitionId::Disable));
  _srv->events(insert(_srv->newDatagram(), TransitionId::EndCalibCycle));
  _srv->events(insert(_srv->newDatagram(), TransitionId::EndRun));
  _srv->events(insert(_srv->newDatagram(), TransitionId::Unconfigure));
}
