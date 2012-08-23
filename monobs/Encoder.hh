#ifndef PdsCas_Encoder_hh
#define PdsCas_Encoder_hh

#include "pdsapp/monobs/Handler.hh"
#include "pds/service/Semaphore.hh"

namespace Pds_Epics { class PVWriter; }

namespace PdsCas {
  class Encoder : public Handler {
  public:
    enum { NCHANNELS=3 };
    Encoder(const char* pvbase, int detid);
    ~Encoder();
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
    Pds_Epics::PVWriter*  _valu_writer;
  };
};

#endif
