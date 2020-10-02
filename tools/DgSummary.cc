#include "DgSummary.hh"

#include "pds/xtc/SummaryDg.hh"
#include "pds/xtc/CDatagramIterator.hh"
#include "pdsdata/psddl/l3t.ddl.h"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
// Event code information
#include "pds/evgr/EvrDefs.hh"
#include "pds/config/EvrConfigType.hh"

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
      printf("BldStats::discover %x %08x.%08x\n",_mask,_src.log(),_src.phy());
#endif
    }
    void fill(SummaryDg::Dg& out) {
      for(unsigned i=0; i<BldInfo::NumberOf; i++)
        if (_mask & (1<<i)) {
          BldInfo info(static_cast<const ProcInfo&>(_src).processId(),BldInfo::Type(i));
          out.append(info,Damage(1<<Damage::DroppedContribution));
        }
    }
    Src src() const { return _src; }
    Src src(const Src& input) const { 
      BldInfo info(static_cast<const ProcInfo&>(_src).processId(),
                   BldInfo::Type(input.phy()));
      return info;
    }
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

  class L3CIterator : public XtcIterator {
  public:
    L3CIterator() : _found(false) {}
  public:
    bool found() const { return _found; }
  public:
    int process(Xtc* xtc) {
      if (xtc->contains.id()==TypeId::Id_Xtc) {
        iterate(xtc);
        return 1;
      }
      if (xtc->contains.id()==TypeId::Id_L3TConfig)
        _found=true;
      return 0;
    }
  private:
    bool _found;
  };

  class BeamEventIterator : public XtcIterator {
  public:
    BeamEventIterator(uint32_t timestamp, unsigned beamPresentCode=EVENT_CODE_HXR) :
      _found(false),
      _beamPresentCode(beamPresentCode),
      _timestamp(timestamp) {}
  public:
    bool found() const { return _found; }
  public:
    int process(Xtc* xtc) {
      if (xtc->contains.id()==TypeId::Id_Xtc) {
        iterate(xtc);
        return 1;
      }
      if (xtc->contains.id()==TypeId::Id_EvrData) {
        const EvrDataType& evrd = *reinterpret_cast<const EvrDataType*>(xtc->payload());
        for(unsigned i=0; i<evrd.numFifoEvents(); i++) {
          const Pds::EvrData::FIFOEvent& fe = evrd.fifoEvents()[i];
          if (fe.timestampHigh() == _timestamp && fe.eventCode() == _beamPresentCode) {
            _found=true; //  A beam-present code is found
            break;
          }
        }
      }
      return 0;
    }
  private:
    bool     _found;
    unsigned _beamPresentCode;
    uint32_t _timestamp;
  };
};

using namespace Pds;


DgSummary::DgSummary() : 
  _dgpool(sizeof(SummaryDg::Dg),16),
  _itpool(sizeof(CDatagramIterator),16),
  _bld   (new BldStats),
  _expect(false)
{
}

DgSummary::~DgSummary() { delete _bld; }

Transition* DgSummary::transitions(Transition* tr) { return tr; }

InDatagram* DgSummary::events     (InDatagram* dg) { 

  _out = new(&_dgpool) SummaryDg::Dg(dg->datagram());

  if (dg->datagram().seq.service()==TransitionId::Configure) {

    _out->datagram().xtc.damage.increase(dg->datagram().xtc.damage.value());

    InDatagramIterator* it = dg->iterator(&_itpool);
    _bld->discover(dg->datagram().xtc, it);
    delete it;

    L3CIterator lit;
    lit.iterate(&dg->datagram().xtc);
    if (lit.found())
      _out->append(Pds::L1AcceptEnv::Pass);
  }
  else {
    if (dg->datagram().xtc.damage.value()) {
#ifdef DBUG
      printf("DgSummary::events dmg %x\n",dg->datagram().xtc.damage.value());
#endif
      BeamEventIterator bit(dg->datagram().seq.stamp().fiducials());
      bit.iterate(&dg->datagram().xtc);
      _expect = bit.found();

      InDatagramIterator* it = dg->iterator(&_itpool);
      iterate(dg->datagram().xtc, it);
      delete it;

#ifdef DBUG
      const SummaryDg::Xtc& s = static_cast<const SummaryDg::Xtc&>(_out->datagram().xtc);
      printf("\tdmg %08x\n",s.damage.value());
      for(unsigned j=0; j<s.nSources(); j++)
        printf("\t%08x.%08x\n", s.source(j).log(), s.source(j).phy());
#endif
    }

    if (dg->datagram().seq.service()==TransitionId::L1Accept) {
      const L1AcceptEnv& l1 = static_cast<const L1AcceptEnv&>(dg->datagram().env);
      _out->append(l1.l3t_result());
    }
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
    // don't count as damage for bld if beam is not present
    if (_expect || xtc.contains.id()==TypeId::Id_Epics) {
      if (BldInfo::Type(xtc.src.phy())==BldInfo::EBeam &&
          xtc.damage.value()==(1<<Damage::UserDefined))
        _out->append(EBeamBPM,0);  // don't count as damage
      else
        _out->append(_bld->src(xtc.src),xtc.damage);
    }
  }
  else if (xtc.src.level() == Level::Source)
    advance = -1;
  else if (xtc.contains.id() == TypeId::Id_Xtc)
    advance = iterate(xtc, iter);

  return advance;
}
