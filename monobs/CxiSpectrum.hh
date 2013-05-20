#ifndef PdsCas_CxiSpectrum_hh
#define PdsCas_CxiSpectrum_hh

#include "pdsapp/monobs/Handler.hh"
#include "pds/epicstools/PVWriter.hh"
#include "pds/epicstools/PVMonitor.hh"
#include "pds/epicstools/PVMonitorCb.hh"
#include "pds/service/Semaphore.hh"

using Pds_Epics::PVWriter;
using Pds_Epics::PVMonitor;

namespace PdsCas {
  class CxiSpectrum : public Handler,
		      public Pds_Epics::PVMonitorCb {
  public:
    CxiSpectrum(const char* pvName,
		unsigned    detinfo);
    ~CxiSpectrum();
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
    void _update_pv ();
  private:
    const char* _pvName;
    enum { MaxPixels=1024 };
    double     _spectrum [MaxPixels];
    double     _pspectrum[MaxPixels];
    unsigned _nev;
    Pds::Semaphore _sem;
    bool _initialized;
    Pds_Epics::PVWriter*  _pvalu_writer;
    Pds_Epics::PVWriter*  _valu_writer;
    Pds_Epics::PVWriter*  _nord_writer;
    Pds_Epics::PVWriter*  _rang_writer;
    Pds_Epics::PVMonitor* _xlo_mon;
    Pds_Epics::PVMonitor* _xhi_mon;
    Pds_Epics::PVMonitor* _ylo_mon;
    Pds_Epics::PVMonitor* _yhi_mon;
    Pds_Epics::PVMonitor* _dedy_mon;
    Pds_Epics::PVMonitor* _e0_mon;
    Pds_Epics::PVMonitor* _y0_mon;
    Pds_Epics::PVMonitor* _ang_mon;

    unsigned _xlo,_xhi,_ylo,_yhi;
    double _sinq;
  };
};

#endif
