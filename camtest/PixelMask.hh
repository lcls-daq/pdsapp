#ifndef Pds_PixelMask_hh
#define Pds_PixelMask_hh

//
//  A class for masking certain pixels in the CameraFex algorithm.
//  The class is meant to allow the lowest level loop over pixels
//  to be done without checking the mask wherever possible.
//

#include "CameraFexConfig.hh"

namespace Pds {

  class PixelMask {
  public:
    PixelMask(const CameraFexConfig& cfg) :
      _p  (&cfg.maskedPixelCoord(0)), 
      _end(&cfg.maskedPixelCoord(cfg.nMaskedPixels))
    { _trm.column = _trm.row = -1; }
    
    const CameraPixelCoord& current() const
    { return (_p < _end) ? *_p : _trm; }

    bool advance() { return ++_p < _end; }
    
  private:
    const CameraPixelCoord *_p, *_end;
    CameraPixelCoord _trm;
  };

};

#endif
