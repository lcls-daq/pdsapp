#include "DgSummary.hh"
#include "SummaryDg.hh"

#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

//#define DBUG

namespace Pds {
  class BldStats : private PdsClient::XtcIterator {
  public:
    BldStats() {}
  public:
    void discover(const Xtc& xtc, InDatagramIterator* iter) {
      _mask = 0;
      iterate(xtc, iter);
#ifdef DBUG
      printf("BldStats::discover %x\n",_mask);
#endif
    }
    void fill(SummaryDg::Dg& out) {
      for(unsigned i=0; i<BldInfo::NumberOf; i++)
	if (_mask & (1<<i))
	  out.append(BldInfo(static_cast<const ProcInfo&>(_src).processId(),
			     BldInfo::Type(i)),
                     Damage(1<<Damage::DroppedContribution));
    }
    const Src& src() const { return _src; }
    const Src& src(const Src& input) const { 
      return BldInfo(static_cast<const ProcInfo&>(_src).processId(),
                     BldInfo::Type(input.phy())); }
  private:
    int process(const Xtc& xtc, InDatagramIterator* iter) {
      if (xtc.contains.id() == TypeId::Id_Xtc) {
	if (xtc.src.level()==Level::Segment)
	  _parent = xtc.src;
	return iterate(xtc, iter);
      }
      else if (xtc.src.level() == Level::Reporter) {
	_src = _parent;
	_mask |= (1<<reinterpret_cast<const BldInfo&>(xtc.src).type());
      }
      return 0;
    }
  private:
    Src      _parent;
    Src      _src;
    unsigned _mask;
  };
};

using namespace Pds;


DgSummary::DgSummary() : 
  _dgpool(sizeof(SummaryDg::Dg),16),
  _itpool(sizeof(ZcpDatagramIterator),16),
  _bld   (new BldStats)
{
}

DgSummary::~DgSummary() { delete _bld; }

Transition* DgSummary::transitions(Transition* tr) { return tr; }

InDatagram* DgSummary::events     (InDatagram* dg) { 

  if (dg->datagram().seq.service()==TransitionId::Configure) {
    InDatagramIterator* it = dg->iterator(&_itpool);
    _bld->discover(dg->datagram().xtc, it);
    delete it;
  }

  _out = new(&_dgpool) SummaryDg::Dg(dg->datagram());
  if (dg->datagram().xtc.damage.value()) {
#ifdef DBUG
    printf("DgSummary::events dmg %x\n",dg->datagram().xtc.damage.value());
#endif
    InDatagramIterator* it = dg->iterator(&_itpool);
    iterate(dg->datagram().xtc, it);
    delete it;
  }
  return _out;
}

int DgSummary::process(const Xtc& xtc, InDatagramIterator* iter)
{
  int advance = 0;

  if (xtc.damage.value() && xtc.src.level() == Level::Segment) {
    if (xtc.src == _bld->src()) {
      if (xtc.sizeofPayload())
	advance = iterate(xtc, iter);
      else
	_bld->fill(*_out);
    }
    else
      _out->append(xtc.src,xtc.damage);
  }
  else if (xtc.damage.value() && xtc.src.level() == Level::Reporter) {
    if (BldInfo::Type(xtc.src.phy())==BldInfo::EBeam &&
        xtc.damage.value()==(1<<Damage::UserDefined))
      _out->append(EBeamBPM,0);  // don't count as damage
    else
      _out->append(_bld->src(xtc.src),xtc.damage);
  }
  else if (xtc.src.level() == Level::Source)
    advance = -1;
  else if (xtc.contains.id() == TypeId::Id_Xtc)
    advance = iterate(xtc, iter);

  return advance;
}
