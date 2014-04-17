#include "MonUtils.hh"

using namespace Pds;

static const int MaxColors = 2;
static int nColors = 1;

static QRgb thermal_color(unsigned i, unsigned n)
{
  i = i*256/n;
  if (i<43) return qRgb(i*6,0,0);
  i -= 43;
  if (i<86) return qRgb(255-i*3,i*3,0);
  i -= 86;
  if (i<86) return qRgb(0,255-i*3,i*3);
  i -= 86;
  if (i<40) return qRgb(i*3,0,255-i*3);
  return qRgb(255,255,255);
}

QColor MonUtils::color(int index, int maxcolors)
{
  /*
  int r = (index <  (maxcolors/2)) ? 255 - index*511/maxcolors : 0;
  int b = (index <= (maxcolors/2)) ? index*511/maxcolors : 511 - index*512/maxcolors;
  int g = (index <= (maxcolors/2)) ? 0 : index*511/maxcolors - 255;
  return QColor(r,g,b);
  */
  return thermal_color(index,maxcolors);
}

QColor MonUtils::color(int index)
{
  return color(index%nColors,nColors);
}

void MonUtils::ncolors(int n) { nColors=n; }
