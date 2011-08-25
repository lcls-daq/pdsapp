#ifndef Pds_StatsTree_hh
#define Pds_StatsTree_hh

#include "pds/client/XtcIterator.hh"
#include "pds/service/LinkedList.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/EbBase.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"

#include <stdio.h>
#include <string.h>

namespace StatsT {
  class NodeStats : public Pds::LinkedList<NodeStats>, public XtcIterator {
    public:
    NodeStats(const Pds::Src& n) : _node(n) { reset(); }
    ~NodeStats() {}

    const Pds::Src& node() const { return _node; }

    //   unsigned damage() const { return _damage; }
    //   unsigned events() const { return _events; }
    //   unsigned size  () const { return _size; }

    void reset() { 
      _damage = _events = _size = 0; 
      memset(_dmgbins,0,sizeof(_dmgbins));
      memset(_usrBitBins,0,sizeof(_usrBitBins));
      NodeStats* n = _list.forward();
      while(n != _list.empty()) {
        n->reset();
        n = n->forward();
      }
    }

    int  accumulate(const Pds::Xtc& xtc, Pds::InDatagramIterator* iter) {
      _events++;
      _size   += xtc.sizeofPayload();
      unsigned dmg = xtc.damage.bits();
      _damage |= dmg;
      for(int i=0; dmg; i++, dmg>>=1)
        if (dmg&1) {
          _dmgbins[i]++;
          if (i==Pds::Damage::UserDefined) {
            _usrBitBins[xtc.damage.userBits()]++;
          }
        }
      return xtc.contains.id()==Pds::TypeId::Id_Xtc ? iterate(xtc,iter) : 0;
    }

    int  process(const Pds::Xtc& xtc, Pds::InDatagramIterator* iter) {
      NodeStats* n = _list.forward();
      while(n != _list.empty()) {
        if (n->node() == xtc.src)
          return n->accumulate(xtc,iter);
        n = n->forward();
      }
      // add a new node
      n = new NodeStats(xtc.src);
      _list.insert(n);
      return n->accumulate(xtc,iter);
    }

    void dump(int indent) const {
      printf("%*c%08x/%08x : dmg 0x%08x  events 0x%x  avg sz 0x%llx\n",
          indent, ' ', _node.log(), _node.phy(),
          _damage, _events, _events ? _size/_events : 0);
      for(int i=0; i<24; i++) {
        if (_dmgbins[i]) {
          printf("%*c%8d : 0x%x\n",indent,' ',i,_dmgbins[i]);
          if (i==Pds::Damage::UserDefined) {
            for (int j=0; j<256; j++) {
              if (_usrBitBins[j]) {
                printf("%*c%12dx%02x : 0x%x\n",indent,' ',0,j,_usrBitBins[j]);
              }
            }
          }
        }
      }
      NodeStats* n = _list.forward();
      while(n != _list.empty()) {
        n->dump(indent+2);
        n = n->forward();
      }
    }
    private:
    Pds::LinkedList<NodeStats> _list;
    Pds::Src       _node;
    unsigned  _damage;
    unsigned  _events;
    unsigned long long  _size;
    unsigned  _dmgbins[24];
    unsigned _usrBitBins[256];
  };
};

using namespace Pds;

class StatsTree : public Appliance {
  public:
    StatsTree() : _pool(sizeof(ZcpDatagramIterator),1) {}
    ~StatsTree() {}

    Transition* transitions(Transition* in) {
      _seq = 0;
      _nPrint = 10;
      switch( in->id() ) {
        case TransitionId::Unmap:
          while(_list.forward() != _list.empty())
            delete _list.remove();
          break;
        case TransitionId::Enable:
        {
          EbBase::printFixups(20);
          StatsT::NodeStats* n = _list.forward();
          while(n != _list.empty()) {
            n->reset();
            n = n->forward();
          }
        }
        break;
        default:
          break;
      }
      return in;
    }
    InDatagram* occurrences(InDatagram* in) { return in; }
    InDatagram* events     (InDatagram* in) {
      const Datagram& dg = in->datagram();
      if (!dg.seq.isEvent()) {
        printf("Transition %02x/%08x\n",dg.seq.service(),dg.seq.stamp().fiducials());
        if (dg.seq.service()==TransitionId::Disable) {
          StatsT::NodeStats* n = _list.forward();
          while(n != _list.empty()) {
            n->dump(6);
            n = n->forward();
          }
        }
        return in;
      }

      static const unsigned rollover = 360;
      if ((_seq > dg.seq.stamp().fiducials()) && (dg.seq.stamp().fiducials()>rollover) && _nPrint) {
        printf("seq %08x followed %08x\n",dg.seq.stamp().fiducials(), _seq);
        _nPrint--;
      }
      _seq = dg.seq.stamp().fiducials();

      InDatagramIterator* iter = in->iterator(&_pool);
      process(dg.xtc,iter);
      delete iter;

      return in;
    }

    int process(const Xtc& xtc, InDatagramIterator* iter) {
      StatsT::NodeStats* n = _list.forward();
      while(n != _list.empty()) {
        if (n->node() == xtc.src)
          return n->accumulate(xtc,iter);
        n = n->forward();
      }
      // add a new node
      n = new StatsT::NodeStats(xtc.src);
      _list.insert(n);
      return n->accumulate(xtc,iter);
    }
  private:
    LinkedList<StatsT::NodeStats> _list;
    GenericPool _pool;
    unsigned _seq;
    unsigned _nPrint;
};

#endif
