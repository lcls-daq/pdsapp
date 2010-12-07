#include "pdsapp/monobs/SxrSpectrum.hh"
#include "pds/epicstools/PVWriter.hh"
#include "pds/epicstools/PVMonitor.hh"

#include "pds/camera/FrameType.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <stdio.h>

using namespace PdsCas;
using Pds_Epics::PVWriter;
using Pds_Epics::PVMonitor;

//
//  Need to add monitors for conversion inputs 
//   and replace nelm writer with a monitor.
//

SxrSpectrum::SxrSpectrum(const char* pvName, unsigned info) :
  Handler   (Pds::DetInfo(-1, 
			  // Pds::DetInfo::Camp, 0, Pds::DetInfo::Opal1000, 0),
			  Pds::DetInfo::Detector((info>>24)&0xff), (info>>16)&0xff, 
			  Pds::DetInfo::Device  ((info>> 8)&0xff), info&0xff),
	     Pds::TypeId::Id_Frame,
	     Pds::TypeId::Id_Opal1kConfig),
  _pvName   (pvName),
  _nev      (0),
  _sem      (Semaphore::FULL),
  _initialized(false),
  _xlo(0),_xhi(200),_ylo(0),_yhi(200)
{
}

void SxrSpectrum::initialize()
{
  char buff[64];
  sprintf(buff,"%s:HISTP",_pvName);
  _pvalu_writer = new PVWriter(buff);
  sprintf(buff,"%s:HIST",_pvName);
  _valu_writer = new PVWriter(buff);
  sprintf(buff,"%s:HIST.NORD",_pvName);
  _nord_writer = new PVWriter(buff);
  sprintf(buff,"%s:ENRG",_pvName);
  _rang_writer = new PVWriter(buff);
  sprintf(buff,"%s:CNTL.A",_pvName);
  _xlo_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:CNTL.B",_pvName);
  _xhi_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:CNTL.C",_pvName);
  _ylo_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:CNTL.D",_pvName);
  _yhi_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:CNTL.E",_pvName);
  _dedy_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:CNTL.F",_pvName);
  _e0_mon   = new PVMonitor(buff,*this);
  sprintf(buff,"%s:CNTL.G",_pvName);
  _y0_mon   = new PVMonitor(buff,*this);

  _initialized = true;
}

SxrSpectrum::~SxrSpectrum()
{
  if (_initialized) {
    delete _pvalu_writer;
    delete _valu_writer;
    delete _nord_writer;
    delete _rang_writer;
    delete _xlo_mon;
    delete _xhi_mon;
    delete _ylo_mon;
    delete _yhi_mon;
    delete _dedy_mon;
    delete _e0_mon;
    delete _y0_mon;
  }
}

static void _order(double xlo, double xhi, unsigned& xl, unsigned& xh)
{
  if (xlo < xhi) {
    xl = (xlo <    0) ? 0    : unsigned(xlo);
    xh = (xhi > 1024) ? 1024 : unsigned(xhi);
  }
  else {
    xl = (xhi <    0) ? 0    : unsigned(xhi);
    xh = (xlo > 1024) ? 1024 : unsigned(xlo);
  }
}

void   SxrSpectrum::updated()
{
  if (!_initialized) return;

  unsigned nb = _valu_writer->data_size()/sizeof(double)-2;
  printf("Resetting spectrum of size %d\n", nb);
  { 
    double* v   =  reinterpret_cast<double*>(_valu_writer->data());
    for(unsigned y=0; y<nb; y++)
      *v++ = 0;
  }
  { 
    double  xlo  = *reinterpret_cast<double*>(_xlo_mon->data());
    double  xhi  = *reinterpret_cast<double*>(_xhi_mon->data());
    double  ylo  = *reinterpret_cast<double*>(_ylo_mon->data());
    double  yhi  = *reinterpret_cast<double*>(_yhi_mon->data());

    _order(xlo,xhi,_xlo,_xhi);
    _order(ylo,yhi,_ylo,_yhi);

    *reinterpret_cast<int*>(_nord_writer->data()) = int(_yhi-_ylo);

    double* v    =  reinterpret_cast<double*>(_rang_writer->data());
    double  y0   = *reinterpret_cast<double*>(_y0_mon->data());
    double  e0   = *reinterpret_cast<double*>(_e0_mon->data());
    double  dedy = *reinterpret_cast<double*>(_dedy_mon->data());

    double e = (double(_ylo) - y0)*dedy + e0;
    for(unsigned y=0; y<nb; y++) {
      *v++ = e;
      e   += dedy;
    }
  }
  _nord_writer->put();

  _rang_writer->put();
  _valu_writer->put();
}

void   SxrSpectrum::_configure(const void* payload, const Pds::ClockTime& t) 
{
  updated();
}

void   SxrSpectrum::_event    (const void* payload, const Pds::ClockTime& t) 
{
  if (!_initialized) return;

  int nb = _yhi-_ylo;

  _sem.take();
  const FrameType& frame = *static_cast<const FrameType*>(payload);
  int32_t o = frame.offset()*(_xhi-_xlo);
  const uint16_t* d = reinterpret_cast<const uint16_t*>(frame.data());
  d += _ylo*frame.width() + _xlo;
  double* p = _spectrum;
  double* q = _pspectrum;
  for(int y=0; y<nb; y++) {
    int32_t v = -o;
    for(unsigned x=_xlo; x<_xhi; x++)
      v += *d++;
    *p++ += *q++ = double(v);
    d += frame.width() - _xhi + _xlo;
  }
  _nev++;
  _sem.give();
}

void   SxrSpectrum::_damaged  () {}

void    SxrSpectrum::update_pv() 
{
  double* v = reinterpret_cast<double*>(_valu_writer->data());
  if (!v) {
    printf("No PV\n");
    return;
  }

  int nb = _yhi-_ylo;

  if (_nev) {
    _sem.take();
    double nev = double(_nev);
    double* p = _spectrum;
    for(int y=0; y<nb; y++) {
      *v++ = *p / nev;
      *p++ = 0;
    }
    _nev=0;
    _sem.give();
    _valu_writer ->put();

    p = _pspectrum;
    v = reinterpret_cast<double*>(_pvalu_writer->data());
    for(int y=0; y<nb; y++)
      *v++ = *p++;
    _pvalu_writer->put();
  }
  else {
    for(int y=0; y<nb; y++)
      *v++ = 0;
  }
}

