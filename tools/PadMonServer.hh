#ifndef PadMonServer_hh
#define PadMonServer_hh

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

  class MyMonitorServer;

  class PadMonServer {
  public:
    enum PadType { CsPad, CsPad140k, Fexamp };

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
    void unconfigure();
  private:
    PadType _t;
    MyMonitorServer* _srv;
  };

};

#endif
