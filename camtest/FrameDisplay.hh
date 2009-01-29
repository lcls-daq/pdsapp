/**display.h
 * Header file for display.cc
 */

#ifndef Pds_FrameDISPLAY_HH
#define Pds_FrameDISPLAY_HH

namespace Pds {

  class Frame;
  class TwoDGaussian;

  class FrameDisplay {
  public:
    FrameDisplay();
    ~FrameDisplay() {}

    void show_frame(const Frame&);
    void show_frame(const Frame&,
		    const TwoDGaussian&);
  };
};

#endif	/* #ifndef __DISPLAY_H */
