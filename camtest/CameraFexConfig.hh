#ifndef Pds_CameraFexConfig_hh
#define Pds_CameraFexConfig_hh

#include "CameraPixelCoord.hh"

#include <new>

namespace Pds {

  class CameraFexConfig {
  public:
    enum Algorithm { RawImage, RegionOfInterest, TwoDGaussianFull, TwoDGaussianROI, TwoDGaussianAndFrame,
		     Sink, NumberOf };
    Algorithm        algorithm;
    CameraPixelCoord regionOfInterestStart;    
    CameraPixelCoord regionOfInterestEnd;
    unsigned         threshold;
    unsigned         nMaskedPixels;

    unsigned size() const;
    CameraPixelCoord& maskedPixelCoord(unsigned);
    const CameraPixelCoord& maskedPixelCoord(unsigned) const;

    static const char* algorithm_title(Algorithm);
  };

};

#endif
