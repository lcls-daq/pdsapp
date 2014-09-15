#include "pdsapp/config/SequenceFactory.hh"

using namespace Pds_ConfigDb::GenericPgp;
using Pds::GenericPgp::CRegister;

SequenceFactory::SequenceFactory(unsigned o) :
  _payload_offset(o)
{
}

SequenceFactory::~SequenceFactory()
{
}

unsigned SequenceFactory::sequence_length() const
{ return _seq.size(); }

unsigned SequenceFactory::payload_length() const
{ return _payload.size(); }

CRegister* SequenceFactory::sequence(CRegister* v) const
{
  unsigned length = _seq.size();
  for(unsigned i=0; i<length; i++)
    *v++ = _seq[i];
  return v;
}

uint32_t* SequenceFactory::payload(uint32_t* v) const
{
  unsigned length = _payload.size();
  for(unsigned i=0; i<length; i++)
    *v++ = _payload[i];
  return v;
}

void SequenceFactory::write(unsigned addr,
                            unsigned value)
{
  _seq.push_back(CRegister(CRegister::RegisterWrite,1,
                           addr,
                           _payload.size()+_payload_offset,
                           0xffffffff));
  _payload.push_back(value);
}

void SequenceFactory::awrite(unsigned addr,
			     unsigned value)
{
  _seq.push_back(CRegister(CRegister::RegisterWriteA,1,
                           addr,
                           _payload.size()+_payload_offset,
                           0xffffffff));
  _payload.push_back(value);
}

void SequenceFactory::iwrite(unsigned addr,
                             unsigned index)
{
  _seq.push_back(CRegister(CRegister::RegisterWrite,1,
                           addr,
                           index,
                           0xffffffff));
}

void SequenceFactory::aiwrite(unsigned addr,
			      unsigned index)
{
  _seq.push_back(CRegister(CRegister::RegisterWriteA,1,
                           addr,
			   index,
                           0xffffffff));
}

void SequenceFactory::read(unsigned addr)
{
  _seq.push_back(CRegister(CRegister::RegisterRead,1,
                           addr,
                           _payload.size()+_payload_offset,
                           0xffffffff));
  _payload.push_back(0);
}

void SequenceFactory::iread (unsigned addr,
                             unsigned index)
{
  _seq.push_back(CRegister(CRegister::RegisterRead,1,
                           addr,
                           index,
                           0xffffffff));
}

void SequenceFactory::verify(unsigned addr,
                             unsigned value,
                             unsigned mask)
{
  _seq.push_back(CRegister(CRegister::RegisterVerify,1,
                           addr,
                           _payload.size()+_payload_offset,
                           mask));
  _payload.push_back(value);
}

void SequenceFactory::iverify(unsigned addr,
                              unsigned index,
                              unsigned mask)
{
  _seq.push_back(CRegister(CRegister::RegisterVerify,1,
                           addr,
                           index,
                           mask));
}

void SequenceFactory::spin(unsigned value)
{
  _seq.push_back(CRegister(CRegister::Spin,1,
                           0,
                           _payload.size()+_payload_offset,
                           0));
  _payload.push_back(value);
}

void SequenceFactory::usleep(unsigned value)
{
  _seq.push_back(CRegister(CRegister::Usleep,1,
                           0,
                           _payload.size()+_payload_offset,
                           0));
  _payload.push_back(value);
}

void SequenceFactory::flush()
{
  _seq.push_back(CRegister(CRegister::Flush,0,0,0,0));
}
