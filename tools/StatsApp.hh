#ifndef Pds_StatsApp_hh
#define Pds_StatsApp_hh

#include "pdsdata/xtc/Src.hh"

#include "pds/client/XtcIterator.hh"
#include "pds/service/LinkedList.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/EbBase.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/CDatagramIterator.hh"
#include "pds/client/Browser.hh"

#include <stdio.h>
#include <string.h>

namespace StatsA {
  class NodeStats : public Pds::LinkedList<NodeStats> {
  public:
    NodeStats(const Pds::Src& n) : _node(n) {}
    ~NodeStats() {}

    const Pds::Src& node() const { return _node; }

    //   unsigned damage() const { return _damage; }
    //   unsigned events() const { return _events; }
    //   unsigned size  () const { return _size; }

    void reset() { 
      _damage = _events = _size = 0; 
      memset(_dmgbins,0,sizeof(_dmgbins));
    }
    void accumulate(const Pds::Xtc& xtc) {
      _events++;
      _size   += xtc.sizeofPayload();
      unsigned dmg = xtc.damage.value();
      _damage |= dmg;
      for(int i=0; dmg; i++, dmg>>=1)
	if (dmg&1)
	  _dmgbins[i]++;
    }
    void dump() const {
      int indent = (Pds::Level::NumberOfLevels-_node.level())*2;
      printf("%*c%08x/%08x : dmg 0x%08x  events 0x%x  avg sz 0x%llx\n",
	     indent, ' ', _node.log(), _node.phy(),
	     _damage, _events, _events ? _size/_events : 0);
      for(int i=0; i<32; i++)
	if (_dmgbins[i])
	  printf("%*c%8d : 0x%x\n",indent,' ',i,_dmgbins[i]);
    }
  private:
    Pds::Src  _node;
    unsigned  _damage;
    unsigned  _events;
    unsigned long long  _size;
    unsigned  _dmgbins[32];
  };
};

using StatsA::NodeStats;
using namespace Pds;

class StatsApp : public Appliance, public XtcIterator {
public:
  StatsApp(const Src& s) : _src(s), _pool(sizeof(CDatagramIterator),1) {}
  ~StatsApp() {}

  Transition* transitions(Transition* in) {
    _seq = 0;
    _nPrint = 10;
    switch( in->id() ) {
    case TransitionId::Map:
      {
	const Allocation& alloc = 
	  reinterpret_cast<const Allocate&>(*in).allocation();
	unsigned nnodes = alloc.nnodes();
	for(unsigned i=0; i<nnodes; i++) {
          const Node& node = *alloc.node(i);
	  const Src& s = node.procInfo();
	  if (s.level()==Level::Control) continue;
	  if (s.level()>=_src.level() && !(s==_src) && s.level()!=Level::Reporter) continue;
	  NodeStats* empty = _list.empty();
	  NodeStats* curr  = _list.forward();
	  while ( curr != empty && curr->node().level()>s.level() )
	    curr = curr->forward();
	  curr->insert(new NodeStats(s));
	}
      }
      break;
    case TransitionId::Unmap:
      while(_list.forward() != _list.empty())
	delete _list.remove();
      break;
    case TransitionId::Enable:
      {
	EbBase::printFixups(20);
	NodeStats* n = _list.forward();
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
	NodeStats* n = _list.forward();
	while(n != _list.empty()) {
	  n->dump();
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
    process(dg.xtc, iter);
    delete iter;

//     if (dg.xtc.damage.value()) {
//       iter = in->iterator(&_pool);
//       int advance;
//       Browser browser(in->datagram(), iter, 0, advance);
//       if (in->datagram().xtc.contains.id() == TypeId::Id_Xtc)
// 	if (browser.iterate() < 0)
// 	  printf("..Terminated.\n");
//       delete iter;
//     }
    return in;
  } 

  int process(const Xtc& xtc, InDatagramIterator* iter) {
    NodeStats* n = _list.forward();
    while(n != _list.empty()) {
      if (n->node() == xtc.src) {
	n->accumulate(xtc);
	return iterate(xtc,iter);
      }
      n = n->forward();
    }
    return 0;
  }
private:
  Src _src;
  LinkedList<NodeStats> _list;
  GenericPool _pool;
  unsigned _seq;
  unsigned _nPrint;
};

#endif
