#ifndef Pds_MonTimeScale_hh
#define Pds_MonTimeScale_hh

#include "qwt_scale_draw.h"

#include <QtCore/QTime>

namespace Pds {
  class MonTimeScale : public QwtScaleDraw {
  public:
    MonTimeScale() {}
    
    virtual QwtText label(double v) const
    {
      time_t t = time_t(v);
      struct tm* tm_ptr = localtime(&t);
      QTime tim(tm_ptr->tm_hour,tm_ptr->tm_min,tm_ptr->tm_sec);
      tim.addMSecs(int(1000*(v-t)));
      return tim.toString();
    }
  private:
  };
};
#endif
