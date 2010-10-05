#ifndef Pds_ShmOutlet_hh
#define Pds_ShmOutlet_hh

#include "pds/utility/OutletWire.hh"
#include "pdsdata/app/XtcMonitorServer.hh"

namespace Pds {
  class Pool;

  class ShmOutlet : public OutletWire,
		    public XtcMonitorServer {
  public:
    ShmOutlet(Outlet& outlet,
	      const char* tag,
	      unsigned sizeofBuffers, 
	      int numberofEvBuffers, 
	      unsigned numberofClients);
    ~ShmOutlet();
  public:
    Transition* forward(Transition* dg);
    Occurrence* forward(Occurrence* dg);
    InDatagram* forward(InDatagram* dg);
  public:
    void bind(NamedConnection, const Ins& node) {}
    void bind(unsigned id, const Ins& node) {}
    void unbind(unsigned id) {}
  private:
    void _copyDatagram  (Dgram* dg, char* b);
    void _deleteDatagram(Dgram* dg);
  private:
    Pool* _pool;
  };
};

#endif
