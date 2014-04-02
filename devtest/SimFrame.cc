#include "pds/utility/Appliance.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"

namespace Pds {
  class SimFrame : public Appliance,
		   public XtcIterator {
  public:
    SimFrame() {}
  public:
    Transition* transitions(Transition* tr) { return tr; }
    InDatagram* events     (InDatagram* dg);
    Occurrence* occurrences(Occurrence* occ) { return occ; }
  public:
    int process(Xtc*);
  private:
    CsPadConfigType _cfg;
    enum { SeedShift=6, NBuffers=(1<<SeedShift), SeedMask=NBuffers-1 };
    unsigned _quad_sz;
    char*    _quad_payload;
  };
};

using namespace Pds;

InDatagram* SimFrame::events(InDatagram* dg)
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

int SimFrame::process(Xtc* xtc)
{
  switch(xtc->contains.id()) {
  case TypeId::Id_Xtc:
    iterate(xtc);
    break;
  case TypeId::Id_CspadElement:
    {
      unsigned seed = rand()>>(32-4*SeedShift);
      const CsPad::DataV1& d = *reinterpret_cast<const CsPad::DataV1*>(xtc->payload());
      for(unsigned iq=0; iq<_cfg.numQuads(); iq++) {
	CsPad::ElementV1& q = const_cast<CsPad::ElementV1&>(d.quads(_cfg,iq));
	memcpy(&q+1, _quad_payload+_quad_sz*((seed>>=SeedShift)&SeedMask), _quad_sz);
      }
    }
    break;
  case TypeId::Id_CspadConfig:
    { 
      const CsPadConfigType& cfg = *reinterpret_cast<CsPadConfigType*>(xtc->payload());
      _cfg = cfg;
      _quad_sz = CsPad::ElementV1::_sizeof(cfg)-sizeof(CsPad::ElementV1);
      _quad_payload = new char[NBuffers*_quad_sz];

      char* q = _quad_payload;
      for(unsigned b=0; b<NBuffers; b++) {
	//  Set the payload
	uint16_t* p = reinterpret_cast<uint16_t*>(q);
	uint16_t off = rand()&0x2fff;
	for(unsigned j=0; j<8; j++)
	  for(unsigned k=0; k<Pds::CsPad::ColumnsPerASIC; k++)
	    for(unsigned m=0; m<Pds::CsPad::MaxRowsPerASIC*2; m++)
	      *p++ = (rand()&0x03ff)+off;
	
        // trailer word
	*p++ = 0;
	*p++ = 0;
	q += _quad_sz;
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
extern "C" Appliance* create() { return new SimFrame; }

extern "C" void destroy(Appliance* p) { delete p; }
