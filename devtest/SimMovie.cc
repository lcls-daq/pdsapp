#include "pds/utility/Appliance.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/camera.ddl.h"

namespace Pds {
  class SimMovie : public Appliance,
		   public XtcIterator {
  public:
    SimMovie() : _next(0) {}
  public:
    Transition* transitions(Transition* tr) { return tr; }
    InDatagram* events     (InDatagram* dg);
    Occurrence* occurrences(Occurrence* occ) { return occ; }
  public:
    int process(Xtc*);
  private:
    unsigned  _next;
  };
};

using namespace Pds;

InDatagram* SimMovie::events(InDatagram* dg)
{
  switch(dg->datagram().seq.service()) {
  case TransitionId::L1Accept:
  case TransitionId::Configure:
    iterate(&dg->datagram().xtc);
    break;
  default:
    break;
  }
  return dg;
}

int SimMovie::process(Xtc* xtc)
{
  switch(xtc->contains.id()) {
  case TypeId::Id_Xtc:
    iterate(xtc);
    break;
  case TypeId::Id_Frame:
    { 
      Camera::FrameV1& frame = *reinterpret_cast<Camera::FrameV1*>(xtc->payload());
      ndarray<const uint16_t,2> f = frame.data16();
      if (_next >= f.size())
        _next = 0;
      const_cast<uint16_t*>(f.begin())[_next++] = 0x1ff;
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
extern "C" Appliance* create() { return new SimMovie; }

extern "C" void destroy(Appliance* p) { delete p; }
