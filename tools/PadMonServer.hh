#ifndef PadMonServer_hh
#define PadMonServer_hh

#include <stdint.h>

namespace Pds {

  //
  //  Pixel array detector data structures
  //
  namespace CsPad  {
    class ConfigV3; 
    class ElementV1;
    class MiniElementV1;
  };
  namespace Fexamp { 
    class ConfigV1;
    class ElementV1;
  };
  namespace Imp {
    class ConfigV1;
    class ElementV1;
  };

  class MyMonitorServer;

  class PadMonServer {
  public:
    enum PadType { CsPad, CsPad140k, Fexamp, Imp, Epix };

    PadMonServer(PadType, const char* tag);
    ~PadMonServer();
  public:
    void configure(const Pds::CsPad::ConfigV3&);
    void event    (const Pds::CsPad::ElementV1&);     // CsPad
    void event    (const Pds::CsPad::MiniElementV1&); // CsPad140k
  public:
    void configure(const Pds::Fexamp::ConfigV1&);
    void event    (const Pds::Fexamp::ElementV1&);    // Fexamp
  public:
    void configure(const Pds::Imp::ConfigV1&);
    void event    (const Pds::Imp::ElementV1&);       // Imp
  public:
    void config_1d(unsigned nsamples);                // reformat into Imp
    void event_1d (const uint16_t*,
		   unsigned nstep);                   // assumes <nstep> channels x nsamples
  public:
    void config_2d(unsigned nasics_x,
		   unsigned nasics_y,
		   unsigned nsamples,
		   unsigned asic_x,
		   unsigned asic_y);                  // EpixTest
    void event_2d (const uint16_t*);
  public:
    void unconfigure();
  private:
    PadType _t;
    MyMonitorServer* _srv;
  };

};

#endif
