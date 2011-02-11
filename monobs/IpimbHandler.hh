#ifndef PdsCas_IpimbHandler_hh
#define PdsCas_IpimbHandler_hh

#include "pdsapp/monobs/Handler.hh"
#include "pds/epicstools/PVWriter.hh"
#include "pds/service/Semaphore.hh"

using Pds_Epics::PVWriter;

namespace Pds { class DetInfo; };

namespace PdsCas {
  class IpimbHandler : public Handler {
  public:
    enum { NDIODES=4 };
    IpimbHandler(const char* pvbase, const Pds::DetInfo&);
    ~IpimbHandler();
  public:
    virtual void   _configure(const void* payload, const Pds::ClockTime& t);
    virtual void   _event    (const void* payload, const Pds::ClockTime& t);
    virtual void   _damaged  ();
  public:
    virtual void    initialize();
    virtual void    update_pv ();
  private:
    enum { PVNAMELEN=32 };
    char _pvName[PVNAMELEN];
    bool _initialized;
    PVWriter*  _valu_writer[NDIODES];
  };
};

#endif
