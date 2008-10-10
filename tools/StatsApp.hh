#ifndef Pds_StatsApp_hh
#define Pds_StatsApp_hh

#include "pds/client/InXtcIterator.hh"
#include "pds/service/LinkedList.hh"
#include "pds/utility/Appliance.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/xtc/Src.hh"

#include <stdio.h>
#include <string.h>

using namespace Pds;

class NodeStats : public LinkedList<NodeStats> {
public:
  NodeStats(const Src& n) : _node(n) {}
  ~NodeStats() {}

  const Src& node() const { return _node; }

//   unsigned damage() const { return _damage; }
//   unsigned events() const { return _events; }
//   unsigned size  () const { return _size; }

  void reset() { 
    _damage = _events = _size = 0; 
    memset(_dmgbins,0,sizeof(_dmgbins));
  }
  void accumulate(const InXtc& xtc) {
    _events++;
    _size   += xtc.sizeofPayload();
    unsigned dmg = xtc.damage.value();
    _damage |= dmg;
    for(int i=0; dmg; i++, dmg>>=1)
      if (dmg&1)
	_dmgbins[i]++;
  }
  void dump() const {
    int indent = _node.level()*2;
    printf("%*c%08x/%08x : dmg 0x%08x  events 0x%x  avg sz 0x%x\n",
	   indent, ' ', _node.log(), _node.phy(),
	   _damage, _events, _size);
    for(int i=0; i<32; i++)
      if (_dmgbins[i])
	printf("%*c%8d : 0x%x\n",indent,' ',i,_dmgbins[i]);
  }
private:
  Src       _node;
  unsigned  _damage;
  unsigned  _events;
  unsigned  _size;
  unsigned  _dmgbins[32];
};

class StatsApp : public Appliance, public InXtcIterator {
public:
  StatsApp() : _list(Src()), _pool(sizeof(ZcpDatagramIterator),1) {}
  ~StatsApp() {}

  Transition* transitions(Transition* in) {
    switch( in->id() ) {
    case TransitionId::Map:
      {
	const Allocate& alloc = reinterpret_cast<const Allocate&>(*in);
	unsigned nnodes = alloc.nnodes();
	for(unsigned i=0; i<nnodes; i++) {
	  Src s(*alloc.node(i));
	  printf("StatsApp: inserting %08x/%08x\n",s.log(),s.phy());
	  _list.insert(new NodeStats(s));
	}
      }
      break;
    case TransitionId::Unmap:
      while(_list.forward() != _list.empty())
	delete _list.remove();
      break;
    case TransitionId::Enable:
      {
	_list.reset();
	NodeStats* n = _list.forward();
	while(n != _list.empty()) {
	  n->reset();
	  n = n->forward();
	}
      }
      break;
    case TransitionId::Disable:
      {
	NodeStats* n = _list.forward();
	while(n != _list.empty()) {
	  n->dump();
	  n = n->forward();
	}
      }
    default:
      break;
    }
    return in;
  }
  InDatagram* occurrences(InDatagram* in) { return in; }
  InDatagram* events     (InDatagram* in) {
    const Datagram& dg = in->datagram();
    if (dg.seq.notEvent()) return in;
    _list.accumulate(dg.xtc);
    InDatagramIterator* iter = in->iterator(&_pool);
    iterate(dg.xtc, iter);
    delete iter;
    return in;
  } 

  int process(const InXtc& xtc, InDatagramIterator* iter) {
    NodeStats* n = _list.forward();
    while(n != _list.empty()) {
      if (n->node() == xtc.src) {
	n->accumulate(xtc);
	return iterate(xtc,iter);
      }
      n = n->forward();
    }
    printf("unrecognized node %08x/%08x\n",xtc.src.log(),xtc.src.phy());
    return iterate(xtc,iter);;
  }
private:
  NodeStats   _list;
  GenericPool _pool;
};

#endif
