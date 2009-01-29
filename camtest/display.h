/**display.h
 * Header file for display.cc
 */

#ifndef __DISPLAY_H
#define __DISPLAY_H

namespace Pds {

  class Frame;
  class TwoDGaussian;

  class Display() {
  public:
    Display();
    ~Display();

    void show_frame(const Frame&);
    void show_frame(const Frame&,
		    const TwoDGaussian&);
  };
};

#endif	/* #ifndef __DISPLAY_H */
