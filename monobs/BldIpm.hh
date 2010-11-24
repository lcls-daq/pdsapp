#ifndef PdsCas_BldIpm_hh
#define PdsCas_BldIpm_hh

#include "pdsapp/monobs/Handler.hh"
#include "pds/service/Semaphore.hh"

namespace PdsCas {
  class PVWriter;

  class BldIpm : public Handler {
  public:
    enum { NDIODES=4 };
    BldIpm(const char* pvbase, int bldid);
    ~BldIpm();
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
    PVWriter*  _sum_writer;
    PVWriter*  _xpos_writer;
    PVWriter*  _ypos_writer;
  };
};

#endif
