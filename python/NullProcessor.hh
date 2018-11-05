#ifndef PdsApp_Python_NullProcessor_hh
#define PdsApp_Python_NullProcessor_hh
//
//  Replaces segment level for pyrogue -> AMI
//
#include "pdsapp/python/FrameProcessor.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Pds {
  class NullProcessor : public FrameProcessor {
  public:
    NullProcessor(unsigned rows, unsigned columns, 
                  unsigned bitdepth, unsigned offset);
    ~NullProcessor();

    Dgram* configure(Dgram*      dg);
    Dgram* event    (Dgram*      dg,
                     const char* source);
  private:
    unsigned _rows, _columns, _bitdepth, _offset;
    DetInfo  _srcInfo;
  };
};

#endif
