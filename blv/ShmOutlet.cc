#include "pdsapp/blv/ShmOutlet.hh"

#include "pds/xtc/CDatagramIterator.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"

using namespace Pds;

ShmOutlet::ShmOutlet(Outlet& outlet,
		     const char* tag,
		     unsigned sizeofBuffers, 
		     int numberofEvBuffers, 
		     unsigned numberofClients) : 
  OutletWire(outlet),
  XtcMonitorServer(tag, sizeofBuffers, numberofEvBuffers, numberofClients),
  _pool           (new GenericPool(sizeof(CDatagramIterator),2))
{
}

ShmOutlet::~ShmOutlet() 
{ 
  delete _pool;
}

Transition* ShmOutlet::forward(Transition* tr) 
{
  printf("ShmOutlet tr %s\n",TransitionId::name(tr->id()));
  return 0;
}

Occurrence* ShmOutlet::forward(Occurrence* dg) { return 0; }
  
InDatagram* ShmOutlet::forward(InDatagram* dg) 
{
  Dgram& dgrm = reinterpret_cast<Dgram&>(dg->datagram());
  return
    (XtcMonitorServer::events(&dgrm) == XtcMonitorServer::Deferred) ?
    (InDatagram*)Pds::Appliance::DontDelete : 0;
}

void ShmOutlet::_copyDatagram(Dgram* dg, char* b) 
{
  Datagram& dgrm = *reinterpret_cast<Datagram*>(dg);
  InDatagram* indg = static_cast<InDatagram*>(&dgrm);

  //  PnccdShuffle::shuffle(dgrm);
  //  CspadShuffle::shuffle(dgrm);

  //  write the datagram
  memcpy(b, &dgrm, sizeof(Datagram));
  //  write the payload
  InDatagramIterator& iter = *indg->iterator(_pool);
  iter.copy(b+sizeof(Datagram), dgrm.xtc.sizeofPayload());
  delete &iter;
}

void ShmOutlet::_deleteDatagram(Dgram* dg)
{
  Datagram& dgrm = *reinterpret_cast<Datagram*>(dg);
  InDatagram* indg = static_cast<InDatagram*>(&dgrm);
  delete indg;
}
