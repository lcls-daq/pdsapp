#ifndef Pds_SummaryDg_hh
#define Pds_SummaryDg_hh

#include "pds/xtc/CDatagram.hh"
#include "pdsdata/xtc/BldInfo.hh"

static Pds::BldInfo EBeamBPM(uint32_t(-1UL),Pds::BldInfo::EBeam);

namespace Pds {
  class SummaryDg : public CDatagram {
  public:
    SummaryDg(const Datagram& dg) : 
      CDatagram(dg,TypeId(TypeId::Any,0),dg.xtc.src),
      _payload(dg.xtc.sizeofPayload())
    {
      datagram().xtc.alloc(sizeof(_payload));
      //      datagram().xtc.damage.increase(dg.xtc.damage.value());
    }
    ~SummaryDg() {}
  public:
    void append(const Src& info, const Damage& dmg) {
      datagram().xtc.damage.increase(dmg.value());
      *static_cast<Src*>(datagram().xtc.alloc(sizeof(info))) = info;
    }
  private:
    unsigned _payload;
    char     _info[32*sizeof(Src)];
  };
};

#endif
