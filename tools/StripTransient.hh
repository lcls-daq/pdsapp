#ifndef Pds_StripTransient_hh
#define Pds_StripTransient_hh

namespace Pds {

  class Dgram;

  class StripTransient {
  public:
    static bool process(Dgram& dg);
  };
  
}
#endif
