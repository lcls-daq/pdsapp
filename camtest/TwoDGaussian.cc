#include "TwoDGaussian.hh"
#include "pds/camera/TwoDMoments.hh"

#include <math.h>

using namespace Pds;

TwoDGaussian::TwoDGaussian(const TwoDMoments& moments,
			   int offset_column,
			   int offset_row)
{
  _xmean = double(moments._x)/double(moments._n);
  _ymean = double(moments._y)/double(moments._n);

  double sxx = double(moments._xx)/double(moments._n) - _xmean*_xmean;
  double syy = double(moments._yy)/double(moments._n) - _ymean*_ymean;
  double sxy = double(moments._xy)/double(moments._n) - _xmean*_ymean;
  double phi  = 0.5*atan2(2*sxy,sxx-syy);
  double cossq = pow(cos(_major_axis_tilt),2);
  double asym  = sxy*sin(2*phi);
  double sxxp = sxx*cossq + syy*(1-cossq) + asym;
  double syyp = syy*cossq + sxx*(1-cossq) - asym;
  if (sxxp > syyp) {
    _major_axis_width = sqrt(sxxp);
    _minor_axis_width = sqrt(syyp);
    _major_axis_tilt  = phi;
  }
  else {
    _major_axis_width = sqrt(syyp);
    _minor_axis_width = sqrt(sxxp);
    _major_axis_tilt  = (phi>0) ? phi-M_PI_2 : phi+M_PI_2;
  }

  _integral = moments._n;
  _xxmoment = moments._xx;
  _yymoment = moments._yy;
  _xymoment = moments._xy;

  _xmean -= double(offset_column);
  _ymean -= double(offset_row);
}
