#include "DgSummary.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/ProcInfo.hh"

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
    void append(const ProcInfo& info) {
      *static_cast<ProcInfo*>(datagram().xtc.alloc(sizeof(info))) = info;
    }
  private:
    unsigned _payload;
    char     _info[32*sizeof(ProcInfo)];
  };
};

using namespace Pds;


DgSummary::DgSummary() : 
  _dgpool(sizeof(SummaryDg),16),
  _itpool(sizeof(ZcpDatagramIterator),16)
{
}

DgSummary::~DgSummary() {}

Transition* DgSummary::transitions(Transition* tr) { return tr; }

InDatagram* DgSummary::events     (InDatagram* dg) { 
  if (dg->datagram().seq.service()==TransitionId::L1Accept) {
    _out = new(&_dgpool) SummaryDg(dg->datagram());
    if (dg->datagram().xtc.damage.value()) {
      InDatagramIterator* it = dg->iterator(&_itpool);
      iterate(dg->datagram().xtc, it);
      delete it;
    }
    return _out;
  }
  return dg;
}

int DgSummary::process(const Xtc& xtc, InDatagramIterator* iter)
{
  if (xtc.contains.id() == TypeId::Id_Xtc)
    return iterate(xtc, iter);

  if (xtc.damage.value()!=0 && xtc.src.level() == Level::Segment)
    _out->append(static_cast<const ProcInfo&>(xtc.src));

  return 0;
}
