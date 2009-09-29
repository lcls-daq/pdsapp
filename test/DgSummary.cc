#include "DgSummary.hh"

#include "pds/xtc/CDatagram.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"

using namespace Pds;

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
private:
  unsigned _payload;
};


DgSummary::DgSummary() : _pool(sizeof(SummaryDg),16) {}

DgSummary::~DgSummary() {}

Transition* DgSummary::transitions(Transition* tr) { return tr; }

InDatagram* DgSummary::events     (InDatagram* dg) { 
  if (dg->datagram().seq.service()==TransitionId::L1Accept)
    return new(&_pool) SummaryDg(dg->datagram());
  return dg;
}
