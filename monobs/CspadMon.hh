#ifndef CspadMon_hh
#define CspadMon_hh

namespace Pds { class DetInfo; }

namespace PdsCas {
  class ShmClient;
  class CspadMon {
  public:
    static void monitor(ShmClient&  client,
                        const char* pvbase,
                        const Pds::DetInfo& info);
    static void monitor(ShmClient&     client,
                        const char*    pvbase,
                        unsigned       detid,
                        unsigned       devid);
  };
}

#endif
