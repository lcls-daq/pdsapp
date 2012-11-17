#include "pdsapp/blv/ToIpmBldEventWire.hh"

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

static const Pds::TypeId inType(Pds::TypeId::Id_SharedIpimb, Pds::BldDataIpimb::version);

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
    if (xtc->contains.id()==TypeId::Id_Xtc)
      iterate(xtc);
    else if (xtc->contains.id()==TypeId::Id_EvrData)
      seq = reinterpret_cast<EvrDatagram*>(xtc->payload())->seq;
    return 1;
  }
public:
  Sequence seq;
};

static const unsigned extent = sizeof(BldDataIpimb);

ToIpmBldEventWire::ToIpmBldEventWire(Outlet&        outlet, 
                                     int            interface, 
                                     int            write_fd,
                                     const std::vector<IpmParams>& ipms) :
  OutletWire(outlet),
  _postman  (interface, Mtu::Size, 1 + 4*(extent+sizeof(Datagram)) / Mtu::Size),
  _ipms     (ipms),
  _write_fd (write_fd),
  _pool     (extent + sizeof(CDatagram),16),
  _task     (new Task(TaskObject("carrier"))),
  _data     (ipms.size()),
  _config   (ipms.size())
{
}

ToIpmBldEventWire::~ToIpmBldEventWire() {}

Transition* ToIpmBldEventWire::forward(Transition* tr) 
{
#ifdef DBUG
  printf("ToIpmBldEventWire::tr %08x.%08x [%p : %d]\n",
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[0],
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[1],
         this, _write_fd);
#endif
  ::write(_write_fd, tr, tr->size());
  return 0; 
}

Occurrence* ToIpmBldEventWire::forward(Occurrence* tr) { return 0; }

#ifdef DBUG
static int _level;
static const char* _nostring = "         ";
#endif

InDatagram* ToIpmBldEventWire::forward(InDatagram* in)
{
  if (in->datagram().seq.service() == TransitionId::L1Accept) {
    SeqFinder finder(&in->datagram().xtc);
    finder.iterate();
    _seq = finder.seq;
#ifdef DBUG
    printf("ToIpmBldEventWire::ev [%p] %08x.%08x [%x]\n",
           in,
           reinterpret_cast<const uint32_t*>(&_seq.stamp())[0],
           reinterpret_cast<const uint32_t*>(&_seq.stamp())[1],
           in->datagram().xtc.sizeofPayload());
#endif    
  }

  if (in->datagram().seq.service() == TransitionId::Configure ||
      in->datagram().seq.service() == TransitionId::L1Accept) {
#ifdef DBUG
    _level = 0;
#endif
    iterate(&in->datagram().xtc);
  }

  if (in->datagram().seq.service() != TransitionId::L1Accept) {
    Transition tr(in->datagram().seq.service(),
                  Transition::Record,
                  in->datagram().seq,
                  in->datagram().env);
#ifdef DBUG
    printf("ToIpmBldEventWire::ev %08x.%08x [%p : %d]\n",
           reinterpret_cast<const uint32_t*>(&tr.sequence().stamp())[0],
           reinterpret_cast<const uint32_t*>(&tr.sequence().stamp())[1],
           this, _write_fd);
#endif
    ::write(_write_fd, (char*)&tr, tr.size());
  }

  return 0;
}

int ToIpmBldEventWire::process(Xtc* xtc)
{
  if (xtc->contains.id()==TypeId::Id_Xtc) {
#ifdef DBUG
    _level++;
#endif
    iterate(xtc);
  }
  else {
#ifdef DBUG
    const unsigned* d = reinterpret_cast<const unsigned*>(xtc->payload());
    printf(" %*s type %x  src %x  ext %x  %08x.%08x.%08x.%08x\n", 
           _level,_nostring,
           xtc->contains.value(), xtc->src.phy(), xtc->extent,
           d[0],d[1],d[2],d[3]);
#endif
    for(unsigned i=0; i<_ipms.size(); i++) {
      if (xtc->src == _ipms[i].det_info) {
        switch(xtc->contains.id()) {
        case TypeId::Id_IpimbConfig:
          _config[i] = *reinterpret_cast<const IpimbConfigV2*>(xtc->payload());
          break;
        case TypeId::Id_IpimbData:
          _data  [i] =  reinterpret_cast<const IpimbDataV2*>(xtc->payload());
          break;
        case TypeId::Id_IpmFex:
          if (_seq.stamp().ticks() && _data[i]) {
            Transition tr(TransitionId::L1Accept,Transition::Record,_seq,Env(0));
            CDatagram* dg = new(&_pool) CDatagram(Datagram(tr,inType,_ipms[i].bld_info));
            const IpmFexDataV1& fex = *reinterpret_cast<const IpmFexDataV1*>(xtc->payload());
            memcpy(dg->xtc.alloc(sizeof(*_data[i])) ,_data[i]   ,sizeof(*_data[i]));
            memcpy(dg->xtc.alloc(sizeof(_config[i])),&_config[i],sizeof(_config[i]));
            memcpy(dg->xtc.alloc(sizeof(fex))       ,&fex       ,sizeof(fex));

            Ins dst(StreamPorts::bld(_ipms[i].bld_info.type()));
            _task->call(new Carrier(dg, dst, _postman, 0));
            _data[i] = 0;
          }
#ifdef DBUG
          else
            printf("TBEW::send:  No sequence\n");
#endif
          break;
        default:
          break;
        }
      }
    }
  }
  return 1;
}

void ToIpmBldEventWire::bind(NamedConnection, const Ins& ins) 
{
}

void ToIpmBldEventWire::bind(unsigned id, const Ins& node) 
{
}

void ToIpmBldEventWire::unbind(unsigned id) 
{
}

void ToIpmBldEventWire::dump(int detail)
{
}

void ToIpmBldEventWire::dumpHistograms(unsigned tag, const char* path)
{
}

void ToIpmBldEventWire::resetHistograms()
{
}
