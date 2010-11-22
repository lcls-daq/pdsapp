#ifndef PDS_TOBLDEVENTWIRE_HH
#define PDS_TOBLDEVENTWIRE_HH

#include <sys/uio.h>
#include "pds/utility/OutletWire.hh"
#include "pds/utility/ToNetEb.hh"
#include "pds/utility/OutletWireInsList.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pds/xtc/XtcType.hh"

namespace Pds {

  class Outlet;
  class Datagram;

  class ToBldEventWire : public OutletWire {
  public:
    ToBldEventWire(Outlet& outlet,int interface, int maxbuf,Ins& destination,
                   unsigned nServers, unsigned* bldIdMap);
    ~ToBldEventWire();

    virtual Transition* forward(Transition* tr);
    virtual Occurrence* forward(Occurrence* tr);
    virtual InDatagram* forward(InDatagram* in);
    virtual void bind(NamedConnection, const Ins& );
    virtual void bind(unsigned id, const Ins& node);
    virtual void unbind(unsigned id);
    
    // Debugging
    virtual void dump(int detail);
    virtual void dumpHistograms(unsigned tag, const char* path);
    virtual void resetHistograms();
	
    bool isempty() const {return _nodes.isempty();}
    
  private:
    OutletWireInsList  _nodes;
    Client       _clientPostman;
    Ins          _bcast;
    Ins&         _destination;
    unsigned*    _bldIdMap; 
    unsigned     _nServers;
    unsigned     _nBldMcast;	
    unsigned     _count;	 	
  };
}

#endif
