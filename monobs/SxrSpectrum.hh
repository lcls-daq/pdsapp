#ifndef PdsCas_SxrSpectrum_hh
#define PdsCas_SxrSpectrum_hh

#include "pdsapp/monobs/Handler.hh"
#include "pdsapp/monobs/PVMonitorCb.hh"
#include "pds/service/Semaphore.hh"

namespace PdsCas {
  class PVWriter;
  class PVMonitor;
  class SxrSpectrum : public Handler,
		      public PVMonitorCb {
  public:
    SxrSpectrum(const char* pvName,
		unsigned    detinfo);
    ~SxrSpectrum();
  public:
    virtual void   _configure(const void* payload, const Pds::ClockTime& t);
    virtual void   _event    (const void* payload, const Pds::ClockTime& t);
    virtual void   _damaged  ();
  public:
    virtual void    initialize();
    virtual void    update_pv ();
  public:
    virtual void    updated   ();
  private:
    const char* _pvName;
    enum { MaxPixels=1024 };
    double     _spectrum [MaxPixels];
    double     _pspectrum[MaxPixels];
    unsigned _nev;
    Pds::Semaphore _sem;
    bool _initialized;
    PVWriter*  _pvalu_writer;
    PVWriter*  _valu_writer;
    PVWriter*  _nord_writer;
    PVWriter*  _rang_writer;
    PVMonitor* _xlo_mon;
    PVMonitor* _xhi_mon;
    PVMonitor* _ylo_mon;
    PVMonitor* _yhi_mon;
    PVMonitor* _dedy_mon;
    PVMonitor* _e0_mon;
    PVMonitor* _y0_mon;

    unsigned _xlo,_xhi,_ylo,_yhi;
  };
};

#endif
