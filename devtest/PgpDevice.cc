#include "pdsapp/devtest/PgpDevice.hh"
#include "pds/pgp/Reg.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/SrpV3.hh"
#include <stdio.h>

using namespace PdsApp;

static bool _verbose = true;

PgpDevice::PgpDevice(Pds::Pgp::SrpV3::Protocol& proto,
                     unsigned dst,
                     unsigned addr,
                     unsigned chunk) : _p(reinterpret_cast<char*>(addr)), _data(new char[chunk]) 
{
  Pds::Pgp::Reg::setPgp(&proto);
  Pds::Pgp::Reg::setDest(dst);
}

PgpDevice::~PgpDevice() { delete[] _data; }

void      PgpDevice::rawWrite(unsigned offset, unsigned v) 
{ if (_verbose) printf("rawWrite [%p]=0x%x\n", _p+offset,v);
  *reinterpret_cast<Pds::Pgp::Reg*>(_p+offset) = v; }

void      PgpDevice::rawWrite(unsigned offset, unsigned* arg1)
{ for(unsigned i=0; i<64; i++) rawWrite(offset+4*i,arg1[i]); }

unsigned  PgpDevice::rawRead (unsigned offset)
{ unsigned v = *reinterpret_cast<Pds::Pgp::Reg*>(_p+offset);
  if (_verbose) printf("rawRead [%p] = %x\n",_p+offset,v);
  return v; }

uint32_t* PgpDevice::rawRead (unsigned offset, unsigned nword)
{ uint32_t* u = reinterpret_cast<uint32_t*>(_data);
  for(unsigned i=0; i<nword; i++) u[i] = rawRead(offset+4*i);
  return u; }
