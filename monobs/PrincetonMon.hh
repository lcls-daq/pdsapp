#ifndef PrincetonMon_hh
#define PrincetonMon_hh

namespace Pds { class DetInfo; }

namespace PdsCas {
  class ShmClient;
  class PrincetonMon {
  public:
    static void monitor(ShmClient&  client,
                        const char* pvbase,
                        const Pds::DetInfo& info);
  };
}

#endif
