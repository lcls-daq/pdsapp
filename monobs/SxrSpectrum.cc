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

SxrSpectrum::SxrSpectrum(const char* pvName) :
  Handler   (Pds::DetInfo(-1, Pds::DetInfo::NoDetector, 0, Pds::DetInfo::Opal1000, 0),
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
  sprintf(buff,"%s:hist",_pvName);
  _valu_writer = new PVWriter(buff);
  sprintf(buff,"%s:cntl.A",_pvName);
  _xlo_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:cntl.B",_pvName);
  _xhi_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:cntl.C",_pvName);
  _ylo_mon = new PVMonitor(buff,*this);
  sprintf(buff,"%s:cntl.D",_pvName);
  _yhi_mon = new PVMonitor(buff,*this);
}

SxrSpectrum::~SxrSpectrum()
{
  delete _valu_writer;
  delete _xlo_mon;
  delete _xhi_mon;
  delete _ylo_mon;
  delete _yhi_mon;
}

void   SxrSpectrum::updated()
{
  unsigned nb = _valu_writer->data_size()/sizeof(double);
  double* v   =  reinterpret_cast<double*>(_valu_writer->data());
  printf("Clearing spectrum of size %d at %p\n", nb, v);
  for(unsigned y=0; y<nb; y++)
    *v++ = 0;
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

  _sem.take();
  double* p = _spectrum;
  const FrameType& frame = *static_cast<const FrameType*>(payload);
  const uint16_t* d = reinterpret_cast<const uint16_t*>(frame.data());
  d += ylo*frame.width() + xlo;
  for(unsigned y=ylo; y<yhi; y++) {
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

  unsigned ylo(unsigned(*reinterpret_cast<double*>(_ylo_mon->data())));
  unsigned yhi(unsigned(*reinterpret_cast<double*>(_yhi_mon->data())));

  if (_nev) {
    _sem.take();
    float  nev = float(_nev);
    double* p = _spectrum;
    for(unsigned y=ylo; y<yhi; y++)
      *v++ = *p++ / nev;
    _sem.give();
  }
  else {
    for(unsigned y=ylo; y<yhi; y++)
      *v++ = 0;
  }
  _valu_writer->put();
}

