/**display.h
 * Header file for display.cc
 */

#ifndef Pds_DISPLAY_HH
#define Pds_DISPLAY_HH

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
