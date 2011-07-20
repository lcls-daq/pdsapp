#include "pdsapp/blv/ToBldEventWire.hh"

#include <errno.h>
#include <unistd.h>
#include <new>

#include "pds/utility/Mtu.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OutletWireHeader.hh"
#include "pds/utility/StreamPorts.hh"

#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Routine.hh"
#include "pdsdata/xtc/TypeId.hh"

#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pdsdata/camera/FrameV1.hh"

//#define DBUG

using namespace Pds;

class Carrier : public Routine {
public: 
  Carrier(CDatagram* dg, const Ins& dst, ToNetEb& postman, unsigned wait_us) :
    _dg(dg), _dst(dst), _postman(postman), _wait_us(wait_us) 
  {}
  ~Carrier() { delete _dg; }
public:
  void routine() {
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000*_wait_us;
    nanosleep(&ts,0);
    _postman.send(_dg, _dst);
    delete this;
  }
private:
  CDatagram* _dg;
  Ins        _dst;
  ToNetEb&   _postman;
  unsigned   _wait_us;
};

class SeqFinder : public XtcIterator {
public:
  SeqFinder(Xtc* xtc) : XtcIterator(xtc) { memset(&seq, 0, sizeof(seq)); }
public:
  int process(Xtc* xtc) {
    if (xtc->contains.id()==TypeId::Id_EvrData)
      seq = reinterpret_cast<EvrDatagram*>(xtc->payload())->seq;
    return 1;
  }
public:
  Sequence seq;
};

// start of Camera Config data
static const unsigned _pulnix_offset  = 2*sizeof(Xtc);

// start of PIM display data
static const unsigned _lusi_offset    = _pulnix_offset  + sizeof(TM6740ConfigType) + sizeof(Xtc);

// start of frame xtc header
static const unsigned _payload_offset = _lusi_offset    + sizeof(PimImageConfigType);

// maximum size
static const unsigned _extent         = 
  _payload_offset + sizeof(Xtc) + sizeof(Camera::FrameV1) + 
  2*Pulnix::TM6740ConfigV2::Row_Pixels*Pulnix::TM6740ConfigV2::Column_Pixels;


ToBldEventWire::ToBldEventWire(Outlet&        outlet, 
                               int            interface, 
                               int            write_fd,
                               const BldInfo& bld,
                               unsigned       wait_us) :
  OutletWire(outlet),
  _postman  (interface, Mtu::Size, 1 + 4*(_extent+sizeof(Datagram)) / Mtu::Size),
  _bld      (bld),
  _write_fd (write_fd),
  _pool     (_extent + sizeof(CDatagram),16),
  _wait_us  (wait_us),
  _task     (new Task(TaskObject("carrier")))
{
  Xtc* xtc = new(new char[_extent]) Xtc(_xtcType,bld);
  {
    Xtc* nxtc = new (xtc->next()) Xtc(_tm6740ConfigType,bld);
    nxtc->extent += sizeof(TM6740ConfigType);
    xtc->extent += nxtc->extent; 
  }
  {
    Xtc* nxtc = new (xtc->next()) Xtc(_pimImageConfigType,bld);
    nxtc->extent += sizeof(PimImageConfigType);
    xtc->extent += nxtc->extent; 
  }
  _xtc = xtc;
}

ToBldEventWire::~ToBldEventWire() {}

Transition* ToBldEventWire::forward(Transition* tr) 
{
#ifdef DBUG
  printf("ToBldEventWire::tr %08x.%08x [%p : %d]\n",
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[0],
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[1],
         this, _write_fd);
#endif
  ::write(_write_fd, tr, tr->size());
  return 0; 
}

Occurrence* ToBldEventWire::forward(Occurrence* tr) { return 0; }

InDatagram* ToBldEventWire::forward(InDatagram* in)
{
  if (in->datagram().seq.service() == TransitionId::L1Accept) {
    SeqFinder finder(&in->datagram().xtc);
    finder.iterate();
    _seq = finder.seq;
#ifdef DBUG
    printf("ToBldEventWire::ev [%p] %08x.%08x [%p : %d]\n",
           in,
           reinterpret_cast<const uint32_t*>(&_seq.stamp())[0],
           reinterpret_cast<const uint32_t*>(&_seq.stamp())[1],
           this, _write_fd);
#endif    
  }

  if (in->datagram().seq.service() == TransitionId::Configure ||
      in->datagram().seq.service() == TransitionId::L1Accept)
    iterate(&in->datagram().xtc);

  if (in->datagram().seq.service() != TransitionId::L1Accept) {
    Transition tr(in->datagram().seq.service(),
                  Transition::Record,
                  in->datagram().seq,
                  in->datagram().env);
#ifdef DBUG
    printf("ToBldEventWire::ev %08x.%08x [%p : %d]\n",
           reinterpret_cast<const uint32_t*>(&tr.sequence().stamp())[0],
           reinterpret_cast<const uint32_t*>(&tr.sequence().stamp())[1],
           this, _write_fd);
#endif
    ::write(_write_fd, (char*)&tr, tr.size());
  }

  return 0;
}

int ToBldEventWire::process(Xtc* xtc)
{
  switch(xtc->contains.id()) {
  case TypeId::Id_Xtc:
    iterate(xtc);
    break;
  case TypeId::Id_TM6740Config:
    _cache(xtc,*reinterpret_cast<const Pulnix::TM6740ConfigV2*>(xtc->payload()));
    break;
  case TypeId::Id_PimImageConfig:
    _cache(xtc,*reinterpret_cast<const Lusi::PimImageConfigV1*>(xtc->payload()));
    break;
  case TypeId::Id_Frame:
    _send(xtc,*reinterpret_cast<const Camera::FrameV1*>(xtc->payload()));
    break;
  default:
    break;
  }
  return 1;
}

//
//  What about damage?
//
void ToBldEventWire::_cache(const Xtc* inxtc,
                            const Pulnix::TM6740ConfigV2& cfg)
{
  Xtc* xtc = _xtc;
  memcpy((char*)xtc + _pulnix_offset, &cfg, sizeof(cfg));
}

void ToBldEventWire::_cache(const Xtc* inxtc,
                            const Lusi::PimImageConfigV1& cfg)
{
  Xtc* xtc = _xtc;
  memcpy((char*)xtc + _lusi_offset, &cfg, sizeof(cfg));
}

void ToBldEventWire::_send(const Xtc* inxtc,
                           const Camera::FrameV1& frame)
{
  if (_seq.stamp().ticks()) {
    Transition tr(TransitionId::L1Accept,Transition::Record,_seq,Env(0));
    CDatagram* dg = new(&_pool) CDatagram(Datagram(tr,_xtcType,Src()));

    Xtc* xtc = _xtc;
    memcpy(&dg->xtc, xtc, xtc->extent);
    {
      Xtc* nxtc = (Xtc*)dg->xtc.alloc(inxtc->extent);
      memcpy(nxtc, inxtc, inxtc->extent);
      nxtc->src = xtc->src;
    }
    Ins dst(StreamPorts::bld(_bld.type()));

    _task->call(new Carrier(dg, dst, _postman, _wait_us));
  }
}

void ToBldEventWire::bind(NamedConnection, const Ins& ins) 
{
}

void ToBldEventWire::bind(unsigned id, const Ins& node) 
{
}

void ToBldEventWire::unbind(unsigned id) 
{
}

void ToBldEventWire::dump(int detail)
{
}

void ToBldEventWire::dumpHistograms(unsigned tag, const char* path)
{
}

void ToBldEventWire::resetHistograms()
{
}
