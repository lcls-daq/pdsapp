#ifndef PdsApp_Python_Epix100aProcessor_hh
#define PdsApp_Python_Epix100aProcessor_hh

//
//  Replaces segment level for pyrogue -> AMI
//

#include "pdsapp/python/FrameProcessor.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Pds {
  class Dgram;
  class Epix100aProcessor : public FrameProcessor {
  public:
    Epix100aProcessor();
    ~Epix100aProcessor();
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
