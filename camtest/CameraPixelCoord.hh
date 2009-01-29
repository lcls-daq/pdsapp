#ifndef Pds_CameraPixelCoord_hh
#define Pds_CameraPixelCoord_hh

namespace Pds {

  class CameraPixelCoord {
  public:
    CameraPixelCoord() {}
    CameraPixelCoord(unsigned short j, unsigned short k) : column(j), row(k) {}
    unsigned short column;
    unsigned short row;
  };

}

#endif
