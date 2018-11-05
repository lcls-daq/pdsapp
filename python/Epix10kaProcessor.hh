#ifndef PdsApp_Python_Epix10kaProcessor_hh
#define PdsApp_Python_Epix10kaProcessor_hh

//
//  Replaces segment level for pyrogue -> AMI
//

#include "pdsapp/python/FrameProcessor.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Pds {
  class Dgram;
  class Epix10kaProcessor : public FrameProcessor {
  public:
    Epix10kaProcessor();
    ~Epix10kaProcessor();
  public:
    Dgram* configure(Dgram*      dg);
    Dgram* event    (Dgram*      dg,
                     const char* source);
  private:
    unsigned    _rows, _columns, _bitdepth, _offset;
    DetInfo     _srcInfo;
    char*       _cfgb;
  };
};

#endif
