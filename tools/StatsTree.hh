#ifndef Pds_StatsTree_hh
#define Pds_StatsTree_hh

#include "pds/service/LinkedList.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/EbBase.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include <stdio.h>
#include <string.h>

namespace StatsT {
  class NodeStats : public Pds::LinkedList<NodeStats>, 
                    public XtcIterator {
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

    int  accumulate(Pds::Xtc& xtc) {
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
      if (xtc.contains.id()==Pds::TypeId::Id_Xtc)
        iterate(&xtc);
      return 1;
    }

    int  process(Pds::Xtc* xtc) {
      NodeStats* n = _list.forward();
      while(n != _list.empty()) {
        if (n->node() == xtc->src)
          return n->accumulate(*xtc);
        n = n->forward();
      }
      // add a new node
      n = new NodeStats(xtc->src);
      _list.insert(n);
      return n->accumulate(*xtc);
    }

    void dump(int indent) const {

      //      if (_damage==0) 
      //        return;

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
  StatsTree() {}
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
    Datagram& dg = in->datagram();
    if (!dg.seq.isEvent()) {
      printf("Transition %02x/%016lx\n",dg.seq.service(),dg.seq.stamp().fiducials());
      if (dg.seq.service()==TransitionId::Disable) {
        StatsT::NodeStats* n = _list.forward();
        while(n != _list.empty()) {
          n->dump(6);
          n = n->forward();
        }
      }
      return in;
    }

    if ((_seq > dg.seq.stamp().fiducials()) && _nPrint) {
      printf("seq %lx followed %lx\n",dg.seq.stamp().fiducials(), _seq);
      _nPrint--;
    }
    _seq = dg.seq.stamp().fiducials();

    process(dg.xtc);

    return in;
  }

  int process(Xtc& xtc) {
    StatsT::NodeStats* n = _list.forward();
    while(n != _list.empty()) {
      if (n->node() == xtc.src)
        return n->accumulate(xtc);
      n = n->forward();
    }
    // add a new node
    n = new StatsT::NodeStats(xtc.src);
    _list.insert(n);
    return n->accumulate(xtc);
  }
private:
  LinkedList<StatsT::NodeStats> _list;
  uint64_t _seq;
  unsigned _nPrint;
};

#endif
