#ifndef Pds_BitCount_hh
#define Pds_BitCount_hh

#include "pdsapp/config/ParameterCount.hh"

#include "pdsapp/config/Parameters.hh"

#include <stdint.h>

namespace Pds_ConfigDb {
  class BitCount : public ParameterCount {
  public:
    BitCount(NumericInt<uint32_t>& mask);
    ~BitCount();
  public:
    bool connect(ParameterSet&);
    unsigned count();
  private:
    NumericInt<uint32_t>& _mask;
  };
};

#endif
