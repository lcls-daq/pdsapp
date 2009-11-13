#include "DgSummary.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/DetInfo.hh"

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
    void append(const DetInfo& det) {
      *static_cast<DetInfo*>(datagram().xtc.alloc(sizeof(det))) = det;
    }
  private:
    unsigned _payload;
    DetInfo  _info[32];
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

  if (xtc.damage.value()==0 && xtc.src.level() == Level::Source) {
    _out->append(static_cast<const DetInfo&>(xtc.src));
    return -1; 
  }

  return 0;
}
