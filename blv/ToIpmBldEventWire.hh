#ifndef PDS_TOIPMBLDEVENTWIRE_HH
#define PDS_TOIPMBLDEVENTWIRE_HH

#include <sys/uio.h>
#include "pds/utility/OutletWire.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include "pds/utility/ToNetEb.hh"
#include "pds/service/GenericPoolW.hh"
#include "pds/service/Task.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpimbDataType.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/Sequence.hh"

#include "pdsdata/psddl/bld.ddl.h"

#include <vector>
#include <string>

namespace Pds {

  class IpmParams {
  public:
    IpmParams() {}
    BldInfo  bld_info;
    DetInfo  det_info;
    std::string port_name;
  };

  class Outlet;

  class ToIpmBldEventWire : public OutletWire,
                            public XtcIterator {
  public:
    ToIpmBldEventWire(Outlet&        outlet,
                      int            interface, 
                      int            write_fd,
                      const std::vector<IpmParams>& ipms);
    ~ToIpmBldEventWire();

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
	
    bool isempty() const {return true;}
  private:
    int process(Xtc* xtc);
  private:
    ToNetEb                           _postman;
    const std::vector<IpmParams>&     _ipms;
    int                               _write_fd;
    GenericPoolW                      _pool;
    Sequence                          _seq;
    Task*                             _task;
    std::vector<const IpimbDataType*> _data;
    std::vector<IpimbConfigType>      _config;
  };
}

#endif
