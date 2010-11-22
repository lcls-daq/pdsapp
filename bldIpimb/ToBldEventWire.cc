#include "ToBldEventWire.hh"

#include <errno.h>
#include "pds/utility/Mtu.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OutletWireHeader.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/Ins.hh"
#include "pdsdata/xtc/TypeId.hh"

using namespace Pds;

ToBldEventWire::ToBldEventWire(Outlet& outlet, int interface, int maxbuf,Ins& destination,
                               unsigned nServers, unsigned* bldIdMap) :
  OutletWire(outlet),
  _clientPostman(sizeof(OutletWireHeader), Mtu::Size, Ins(interface), 1 + maxbuf / Mtu::Size),
  _destination(destination),
  _bldIdMap(bldIdMap),
  _nServers(nServers),  
  _nBldMcast(0),_count(0)
{

}

ToBldEventWire::~ToBldEventWire() {}

Transition* ToBldEventWire::forward(Transition* tr) { return 0; }

Occurrence* ToBldEventWire::forward(Occurrence* tr) { return 0; }

InDatagram* ToBldEventWire::forward(InDatagram* in)
{
  Datagram& dg = in->datagram();
  //printf("In InDatagram* ToBldEventWire::forward xtcType = %s xtcSz = %d dg.xtc.extent = %d \n",Pds::TypeId::name(dg.xtc.contains.id()),sizeof(Xtc),dg.xtc.extent );	 
  int result = 0; 
  int remaining = dg.xtc.sizeofPayload();
  if (remaining <= 0) { printf("ToBldEventWire::forward() Empty XTC Dg \n"); return 0; }
    
  // XTC Interations to separate individual BLD IPIMB data  
  if(dg.xtc.contains.id() == TypeId::Id_Xtc) {
    Xtc* xtc = (Xtc*) dg.xtc.payload();
    _nBldMcast = 0;	
    for(unsigned i=0;((i<_nServers) && (remaining>0));i++) {
      if(xtc->contains.id() == TypeId::Id_SharedIpimb) {
        //*((unsigned*)xtc->payload()) = ++_count; 
        Ins dest(_destination.address()+*(_bldIdMap+i),_destination.portId());
        result = _clientPostman.send((char*) 0, (char*)xtc->payload(), xtc->sizeofPayload(), dest);
        if (result) _log(in->datagram(), result);		
        _nBldMcast++;
      } else {
        printf("*** ToBldEventWire::forward(In): Expected Xtc: Id_SharedIpimb. Received =%s \n",Pds::TypeId::name(xtc->contains.id()));
      }
      remaining -= xtc->sizeofPayload()+ sizeof(Xtc);
      xtc = xtc->next();
    }
    if ( _nBldMcast != _nServers) 
      printf("*** ToBldEventWire::forward(In): Expected %u BLD messages. Sent: %u \n",_nServers,_nBldMcast); 	
	
  } else printf("*** ToBldEventWire::forward() Err: Unknown Xtc ID =%s \n",Pds::TypeId::name(dg.xtc.contains.id()));

  return 0;
}

void ToBldEventWire::bind(NamedConnection, const Ins& ins) 
{
  _bcast = ins;
}

void ToBldEventWire::bind(unsigned id, const Ins& node) 
{
  _nodes.insert(id, node);
}

void ToBldEventWire::unbind(unsigned id) 
{
  _nodes.remove(id);
}

void ToBldEventWire::dump(int detail)
{
  if (!_nodes.isempty()) {
    unsigned i=0;
    OutletWireIns* first = _nodes.lookup(i);
    OutletWireIns* current = first;
    do {
      printf(" Event id %i, port %i, address 0x%x\n",
	     current->id(),
	     current->ins().portId(),
	     current->ins().address());
         //_ack_handler.dump(i, detail);
      current = _nodes.lookup(++i);
    } while (current != first);
  }
}

void ToBldEventWire::dumpHistograms(unsigned tag, const char* path)
{
  if (!_nodes.isempty()) {
    unsigned i=0;
    OutletWireIns* first = _nodes.lookup(i);
    OutletWireIns* current = first;
    do {
      // _ack_handler.dumpHistograms(i, tag, path);
      current = _nodes.lookup(++i);
    } while (current != first);
  }
}

void ToBldEventWire::resetHistograms()
{
  if (!_nodes.isempty()) {
    unsigned i=0;
    OutletWireIns* first = _nodes.lookup(i);
    OutletWireIns* current = first;
    do {
      // _ack_handler.resetHistograms(i);
      current = _nodes.lookup(++i);
    } while (current != first);
  }
}
