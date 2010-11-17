#ifndef CspadMon_hh
#define CspadMon_hh

namespace Pds { class DetInfo; }

namespace PdsCas {
  class ShmClient;
  class CspadMon {
  public:
    static void monitor(ShmClient& client,
                        const Pds::DetInfo& info);
  };
}

#endif
