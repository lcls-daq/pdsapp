#include "SimCam.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/psddl/camera.ddl.h"

template <class T>
static void _stuffIt(Pds::Camera::FrameV1& f)
{
  static unsigned _p = 0x55555555;
  T* d = (T*)(&f+1) + f.height()/4*f.width();
  const T v = _p&((1<<f.depth())-1);
  printf("stuff %x\n",v);
  for(unsigned i=0; i<f.width(); i++)
    *d++ = v;
  
  unsigned p = _p>>1;
  _p = (p) | (_p<<31);
}


using namespace Pds;

SimCam::SimCam()
{
}

SimCam::~SimCam()
{
}

Transition* SimCam::transitions(Transition* tr)
{
  return tr;
}

InDatagram* SimCam::events(InDatagram* dg)
{
  if (dg->datagram().seq.service()==TransitionId::L1Accept) {
    SimCam::MyIter it;
    it.iterate(&dg->datagram().xtc);
  }
  return dg;
}

Occurrence* SimCam::occurrences(Occurrence* occ) {
  return occ;
}

int SimCam::MyIter::process(Xtc* xtc)
{
  if (xtc->contains.id()==TypeId::Id_Xtc)
    iterate(xtc);
  else if (xtc->contains.id()==TypeId::Id_Frame) {
    Camera::FrameV1& f = *reinterpret_cast<Camera::FrameV1*>(xtc->payload());
    switch((f.depth()+7)/8) {
    case 1: _stuffIt<uint8_t >(f); break;
    case 2: _stuffIt<uint16_t>(f); break;
    default:
      break;
    }
  }
  return 0;
}


//
//  Plug-in module creator
//

extern "C" Appliance* create() { return new SimCam; }

extern "C" void destroy(Appliance* p) { delete p; }
