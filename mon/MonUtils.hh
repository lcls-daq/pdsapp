#ifndef Pds_MonUtils_hh
#define Pds_MonUtils_hh

#include <QtGui/QColor>

namespace Pds {
  class MonUtils {
  public:
    static QColor color(int);
    static QColor color(int,int);
    static void   ncolors(int);
  };
};

#endif
