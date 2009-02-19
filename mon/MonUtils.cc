#include "MonUtils.hh"

using namespace Pds;

static const int MaxColors = 2;

QColor MonUtils::color(int index, int maxcolors)
{
  int r = (index <  (maxcolors/2)) ? 255 - index*511/maxcolors : 0;
  int b = (index <= (maxcolors/2)) ? index*511/maxcolors : 511 - index*512/maxcolors;
  int g = (index <= (maxcolors/2)) ? 0 : index*511/maxcolors - 255;
  return QColor(r,g,b);
}

QColor MonUtils::color(int index)
{
  return color(index%MaxColors,MaxColors);
}
