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

#include "pdsdata/bld/bldData.hh"

static const Pds::TypeId inType(Pds::TypeId::Id_SharedPim, Pds::BldDataPimV1::version);

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

static const unsigned extent = sizeof(Pds::BldDataPimV1) +
  Pds::Pulnix::TM6740ConfigV2::Row_Pixels*Pds::Pulnix::TM6740ConfigV2::Column_Pixels*2;

ToBldEventWire::ToBldEventWire(Outlet&        outlet, 
                               int            interface, 
                               int            write_fd,
                               const BldInfo& bld,
                               unsigned       wait_us) :
  OutletWire(outlet),
  _postman  (interface, Mtu::Size, 1 + 4*(extent+sizeof(Datagram)) / Mtu::Size),
  _bld      (bld),
  _write_fd (write_fd),
  _pool     (extent + sizeof(CDatagram),16),
  _wait_us  (wait_us),
  _task     (new Task(TaskObject("carrier")))
{
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
    printf("ToBldEventWire::ev [%p] %08x.%08x [%x]\n",
           in,
           reinterpret_cast<const uint32_t*>(&_seq.stamp())[0],
           reinterpret_cast<const uint32_t*>(&_seq.stamp())[1],
           in->datagram().xtc.sizeofPayload());
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
    _camConfig = *reinterpret_cast<const Pulnix::TM6740ConfigV2*>(xtc->payload());
    break;
  case TypeId::Id_PimImageConfig:
    _pimConfig = *reinterpret_cast<const Lusi::PimImageConfigV1*>(xtc->payload());
    break;
  case TypeId::Id_Frame:
    _send(xtc,*reinterpret_cast<const Camera::FrameV1*>(xtc->payload()));
    break;
  default:
    break;
  }
  return 1;
}

void ToBldEventWire::_send(const Xtc* inxtc,
                           const Camera::FrameV1& frame)
{
  if (_seq.stamp().ticks()) {
    Transition tr(TransitionId::L1Accept,Transition::Record,_seq,Env(0));
    CDatagram* dg = new(&_pool) CDatagram(Datagram(tr,inType,_bld));

    memcpy(dg->xtc.alloc(sizeof(_camConfig)),&_camConfig,sizeof(_camConfig));
    memcpy(dg->xtc.alloc(sizeof(_pimConfig)),&_pimConfig,sizeof(_pimConfig));
    unsigned frame_size = sizeof(frame) + frame.data_size();
    memcpy(dg->xtc.alloc(frame_size),&frame,frame_size);

    Ins dst(StreamPorts::bld(_bld.type()));

    _task->call(new Carrier(dg, dst, _postman, _wait_us));
  }
#ifdef DBUG
  else
    printf("TBEW::send:  No sequence");
#endif
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
