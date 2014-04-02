#include "pds/utility/Appliance.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <stdlib.h>
#include <fcntl.h>

namespace Pds {
  class PlayFrame : public Appliance,
		    public XtcIterator {
  public:
    PlayFrame() {}
  public:
    Transition* transitions(Transition* tr) { return tr; }
    InDatagram* events     (InDatagram* dg);
    Occurrence* occurrences(Occurrence* occ) { return occ; }
  public:
    int process(Xtc*);
  private:
    CsPadConfigType _cfg;
    //enum { SeedShift=6, NBuffers=(1<<SeedShift), SeedMask=NBuffers-1 };
    enum { SeedShift=9, NBuffers=(1<<SeedShift), SeedMask=NBuffers-1 };
    unsigned _quad_sz;
    char*    _quad_payload;
  };

  class XtcPlay : public XtcIterator {
  public:
    XtcPlay(const char*   fname,
	    const Src&    src,
	    const TypeId& id) :
      _iter(_fd = open(fname,O_RDONLY|O_LARGEFILE), 0x1400000), _src(src), _id(id)
    { 
      if (_fd<0) {
	perror("Failed to open xtc file");
	exit(1);
      }
    }
  public:
    const Xtc* next() {
      _xtc = 0;

      Dgram* dg;
      while( (dg = _iter.next()) &&
	     dg->seq.service()!=TransitionId::L1Accept ) ;
      if (!dg) return 0;

      iterate(&dg->xtc);

      return _xtc;
    }
    int process(Xtc* xtc);
  private:
    int             _fd;
    XtcFileIterator _iter;
    const Src&      _src;
    const TypeId&   _id;
    const Xtc*      _xtc;
  };
};

using namespace Pds;

InDatagram* PlayFrame::events(InDatagram* dg)
{
  switch(dg->datagram().seq.service()) {
  case TransitionId::L1Accept:
    iterate(&dg->datagram().xtc);
    break;
  case TransitionId::Configure:
    {
      iterate(&dg->datagram().xtc);

      post(dg);

      char buff[256];
      sprintf(buff,"%s/play.xtc",getenv("HOME"));
      XtcPlay play(buff,
		   DetInfo(0,DetInfo::CxiDs1,0,DetInfo::Cspad,0),
		   TypeId(TypeId::Id_CspadElement,2));
      const Xtc* pxtc = play.next();
      char* pq = pxtc->payload()+sizeof(CsPad::ElementV1);

      char* q = _quad_payload;
      for(unsigned b=0; b<NBuffers; b++) {
	if (pq-pxtc->payload() >= pxtc->extent) {
	  pxtc = play.next();
	  pq   = pxtc->payload()+sizeof(CsPad::ElementV1);
	}
	memcpy(q, pq, _quad_sz);
	uint16_t* p = reinterpret_cast<uint16_t*>(q);
	q  += _quad_sz;
#if 0
	while( p < (uint16_t*)q) {
	  *p++ <<= 1;
	}
#endif
	pq += CsPad::ElementV1::_sizeof(_cfg);
      }
      return (InDatagram*)Appliance::DontDelete;
    }
  default:
    break;
  }
  return dg;
}

int PlayFrame::process(Xtc* xtc)
{
  switch(xtc->contains.id()) {
  case TypeId::Id_Xtc:
    iterate(xtc);
    break;
  case TypeId::Id_CspadElement:
    {
      //unsigned seed = rand()>>(32-4*SeedShift);
      unsigned seed = rand();
      unsigned b = ((seed>>=SeedShift)&SeedMask)&~3;
      const CsPad::DataV1& d = *reinterpret_cast<const CsPad::DataV1*>(xtc->payload());
      for(unsigned iq=0; iq<_cfg.numQuads(); iq++) {
	CsPad::ElementV1& q = const_cast<CsPad::ElementV1&>(d.quads(_cfg,iq));
	//memcpy(&q+1, _quad_payload+_quad_sz*((seed>>=SeedShift)&SeedMask), _quad_sz);
	memcpy(&q+1, _quad_payload+_quad_sz*b++, _quad_sz);
      }
    }
    break;
  case TypeId::Id_CspadConfig:
    { 
      const CsPadConfigType& cfg = *reinterpret_cast<CsPadConfigType*>(xtc->payload());
      _cfg = cfg;
      _quad_sz = CsPad::ElementV1::_sizeof(cfg)-sizeof(CsPad::ElementV1);
      _quad_payload = new char[NBuffers*_quad_sz];
    }

    break;
  default:
    break;
  }
  return 1;
}

int XtcPlay::process(Xtc* xtc)
{
  if (xtc->contains.id()==TypeId::Id_Xtc) {
    iterate(xtc);
  }
  else if (xtc->contains.value() == _id.value() && 
	   xtc->src.phy() == _src.phy()) {
    _xtc = xtc;
    return 0;
  }
  return 1;
}

//
//  Plug-in module creator
//
extern "C" Appliance* create() { return new PlayFrame; }

extern "C" void destroy(Appliance* p) { delete p; }
