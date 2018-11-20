#include "pdsapp/devtest/MMDevice.hh"
#include <stdio.h>

using namespace PdsApp;

static bool _verbose = false;

MMDevice::MMDevice(char*    base,
                   unsigned chunk) : _p(base), _data(new char[chunk]) {}

MMDevice::~MMDevice() { delete[] _data; }

void      MMDevice::rawWrite(unsigned offset, unsigned v) 
{ if (_verbose) printf("rawWrite [%p]=0x%x\n", _p+offset,v);
  *reinterpret_cast<volatile uint32_t*>(_p+offset) = v; }

unsigned  MMDevice::rawRead (unsigned offset)
{ if (_verbose) printf("rawRead [%p]\n",_p+offset);
  volatile unsigned v = *reinterpret_cast<volatile uint32_t*>(_p+offset);
  return v; }

void      MMDevice::rawWrite(unsigned offset, unsigned* arg1)
{ for(unsigned i=0; i<64; i++) rawWrite(offset+4*i,arg1[i]); }

uint32_t* MMDevice::rawRead (unsigned offset, unsigned nword)
{ uint32_t* u = reinterpret_cast<uint32_t*>(_data);
  for(unsigned i=0; i<nword; i++) u[i] = rawRead(offset+4*i);
  return u; }
