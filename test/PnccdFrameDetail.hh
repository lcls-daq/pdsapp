#ifndef PNCCDFRAMEDETAIL_HH
#define PNCCDFRAMEDETAIL_HH

#include <stdint.h>

namespace Pds {
  namespace PNCCD {
    class Camex {
    public:
      enum {NumChan=128};
      uint16_t data[NumChan];
    };

    class Line {
    public:
      enum {NumCamex=4};
      Camex cmx[NumCamex];
    }; 

    class Image {
    public:
      enum {NumLines=512};
      Line line[NumLines];
    };
  }
}

#endif
