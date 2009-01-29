#ifndef Pds_Opal1kConfig_hh
#define Pds_Opal1kConfig_hh

namespace Pds {

  class CameraPixelCoord;

  class Opal1kConfig {
  public:
    unsigned Depth_bits;
    unsigned Gain_percent;
    unsigned BlackLevel_percent;
    unsigned ShutterWidth_us;
    unsigned nDefectPixels;

    const CameraPixelCoord& DefectPixelCoord(unsigned) const;
    unsigned size() const;
  };

};

#endif
