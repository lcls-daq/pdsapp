#ifndef Pds_SummaryDg_hh
#define Pds_SummaryDg_hh

#include "pds/xtc/CDatagram.hh"
#include "pdsdata/xtc/BldInfo.hh"

static Pds::BldInfo EBeamBPM(uint32_t(-1UL),Pds::BldInfo::EBeam);

namespace Pds {
  class SummaryDg : public CDatagram {
  public:
    enum L3TResult { None, Pass, Fail };
    static TypeId typeId() { return Pds::TypeId(TypeId::Any,1); }
  public:
    SummaryDg(const Datagram& dg);
    ~SummaryDg() {}
  public:
    void append(const Src& info, const Damage& dmg);
    void append(L3TResult v);
  public:
    unsigned payload() const;
    L3TResult l3tresult() const;
    unsigned nSources() const;
    const Src& source(unsigned i) const;
  private:
    uint32_t _payload;
    char     _info[32*sizeof(Src)];
  };
};

inline unsigned Pds::SummaryDg::payload() const 
{ return _payload&0x3fffffff; }

inline Pds::SummaryDg::L3TResult Pds::SummaryDg::l3tresult() const
{ return L3TResult(_payload>>30); }

inline unsigned Pds::SummaryDg::nSources() const
{ return (datagram().xtc.extent-sizeof(_payload))/sizeof(Pds::Src); }

inline const Pds::Src& Pds::SummaryDg::source(unsigned i) const
{ return *reinterpret_cast<const Src*>(&_info[i*sizeof(Pds::Src)]); }

inline Pds::SummaryDg::SummaryDg(const Datagram& dg) : 
  CDatagram(dg,typeId(),dg.xtc.src),
  _payload(dg.xtc.sizeofPayload())
{
  datagram().xtc.alloc(sizeof(_payload));
}

inline void Pds::SummaryDg::append(const Src& info, const Damage& dmg) 
{
  datagram().xtc.damage.increase(dmg.value());
  *static_cast<Src*>(datagram().xtc.alloc(sizeof(info))) = info;
}

inline void Pds::SummaryDg::append(L3TResult v)
{ _payload |= (unsigned(v)<<30); }

#endif
