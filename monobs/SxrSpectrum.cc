#include "pdsapp/monobs/SxrSpectrum.hh"
#include "pdsapp/monobs/PVWriter.hh"
#include "pdsapp/monobs/PVMonitor.hh"

#include "pds/camera/FrameType.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <stdio.h>

using namespace PdsCas;

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
  _sem      (Semaphore::FULL)
{
}

void SxrSpectrum::initialize()
{
  char buff[64];
  sprintf(buff,"%s:HIST",_pvName);
  _valu_writer = new PVWriter(buff);
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
}

SxrSpectrum::~SxrSpectrum()
{
  delete _valu_writer;
  delete _rang_writer;
  delete _xlo_mon;
  delete _xhi_mon;
  delete _ylo_mon;
  delete _yhi_mon;
  delete _dedy_mon;
  delete _e0_mon;
}

void   SxrSpectrum::updated()
{
  unsigned nb = _valu_writer->data_size()/sizeof(double)-2;
  printf("Resetting spectrum of size %d\n", nb);
  { 
    double* v   =  reinterpret_cast<double*>(_valu_writer->data());
    for(unsigned y=0; y<nb; y++)
      *v++ = 0;
  }
  { 
    double* v    =  reinterpret_cast<double*>(_rang_writer->data());
    double  ylo  = *reinterpret_cast<double*>(_ylo_mon->data());
    double  e0   = *reinterpret_cast<double*>(_e0_mon->data());
    double  dedy = *reinterpret_cast<double*>(_dedy_mon->data());
    e0 += ylo * dedy;
    for(unsigned y=0; y<nb; y++) {
      *v++ = e0;
      e0  += dedy;
    }
  }
  _rang_writer->put();
  _valu_writer->put();
}

void   SxrSpectrum::_configure(const void* payload, const Pds::ClockTime& t) 
{
  updated();
}

void   SxrSpectrum::_event    (const void* payload, const Pds::ClockTime& t) 
{
  unsigned xlo(unsigned(*reinterpret_cast<double*>(_xlo_mon->data())));
  unsigned xhi(unsigned(*reinterpret_cast<double*>(_xhi_mon->data())));
  unsigned ylo(unsigned(*reinterpret_cast<double*>(_ylo_mon->data())));
  unsigned yhi(unsigned(*reinterpret_cast<double*>(_yhi_mon->data())));
  unsigned nb = _valu_writer->data_size()/sizeof(double);
  if (yhi < ylo+nb) 
    nb = yhi-ylo;

  _sem.take();
  double* p = _spectrum;
  const FrameType& frame = *static_cast<const FrameType*>(payload);
  const uint16_t* d = reinterpret_cast<const uint16_t*>(frame.data());
  d += ylo*frame.width() + xlo;
  for(unsigned y=0; y<nb; y++) {
    int32_t v = 0;
    for(unsigned x=xlo; x<xhi; x++)
      v += *d++;
    *p++ = double(v);
    d += frame.width() - xhi + xlo;
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

  unsigned nb = _valu_writer->data_size()/sizeof(double);

  if (_nev) {
    _sem.take();
    float  nev = float(_nev);
    double* p = _spectrum;
    for(unsigned y=0; y<nb; y++) {
      *v++ = *p / nev;
      *p++ = 0;
    }
    _nev=0;
    _sem.give();
  }
  else {
    for(unsigned y=0; y<nb; y++)
      *v++ = 0;
  }
  _valu_writer->put();
}

