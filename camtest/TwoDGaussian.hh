#ifndef Pds_TwoDGaussian_hh
#define Pds_TwoDGaussian_hh

namespace Pds {
  class TwoDMoments;
  class TwoDGaussian {
  public:
    TwoDGaussian() {}
    TwoDGaussian(const TwoDMoments& moments,
		 int offset_column=0,
		 int offset_row=0);
    ~TwoDGaussian() {}

    double _xmean;
    double _ymean;
    double _major_axis_width;
    double _minor_axis_width;
    double _major_axis_tilt;
    unsigned long long _integral;
    unsigned long long _xxmoment;
    unsigned long long _yymoment;
    unsigned long long _xymoment;
  };
};

#endif
