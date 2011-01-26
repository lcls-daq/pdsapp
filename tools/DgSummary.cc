#include "DgSummary.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

namespace Pds {
  class SummaryDg : public CDatagram {
  public:
    SummaryDg(const Datagram& dg) : 
      CDatagram(dg,TypeId(TypeId::Any,0),dg.xtc.src),
      _payload(dg.xtc.sizeofPayload())
    {
      datagram().xtc.alloc(sizeof(_payload));
      datagram().xtc.damage.increase(dg.xtc.damage.value());
    }
    ~SummaryDg() {}
  public:
    void append(const Src& info) {
      *static_cast<Src*>(datagram().xtc.alloc(sizeof(info))) = info;
    }
  private:
    unsigned _payload;
    char     _info[32*sizeof(Src)];
  };

  class BldStats : private XtcIterator {
  public:
    BldStats() {}
  public:
    void discover(const Xtc& xtc, InDatagramIterator* iter) {
      _mask = 0;
      iterate(xtc, iter);
    }
    void fill(SummaryDg& out) {
      for(unsigned i=0; i<BldInfo::NumberOf; i++)
	if (_mask & (1<<i))
	  out.append(BldInfo(static_cast<const ProcInfo&>(_src).processId(),
			     BldInfo::Type(i)));
    }
    const Src& src() const { return _src; }
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
  _dgpool(sizeof(SummaryDg),16),
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

  _out = new(&_dgpool) SummaryDg(dg->datagram());
  if (dg->datagram().xtc.damage.value()) {
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
      _out->append(xtc.src);
  }
  else if (xtc.damage.value() && xtc.src.level() == Level::Reporter)
    _out->append(xtc.src);
  else if (xtc.src.level() == Level::Source)
    advance = -1;
  else if (xtc.contains.id() == TypeId::Id_Xtc)
    advance = iterate(xtc, iter);

  return advance;
}
