#ifndef Pds_JungfrauSegBuilder_hh
#define Pds_JungfrauSegBuilder_hh

namespace Pds {

  class Dgram;
  class ProcInfo;

  class JungfrauSegBuilder {
  public:
    static bool build(Dgram& dg);
    static bool build(Dgram& dg, const ProcInfo& proc);
  private:
    static bool process(Dgram& dg, const ProcInfo& proc);
  };
  
}
#endif
