#include "pdsapp/config/EvrConfigType_V6.hh"

using namespace Pds_ConfigDb::EvrConfig_V6;

unsigned Pds_ConfigDb::EvrConfig_V6::size(const EvrConfigType& tc)
{
  return sizeof(tc) +
    tc.neventcodes()*sizeof(EventCodeType) +
    tc.npulses    ()*sizeof(PulseType) +
    tc.noutputs   ()*sizeof(OutputMapType) +
    tc.seq_config ()._sizeof();
}

