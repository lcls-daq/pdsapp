#ifndef Pds_QtPlotCurve_hh
#define Pds_QtPlotCurve_hh

#include <QtCore/QObject>
#include "qwt_plot_curve.h"

namespace Pds {
  class QtPlotCurve : public QObject,
                      public QwtPlotCurve {
    Q_OBJECT
  public:
    QtPlotCurve(const char* name);
    ~QtPlotCurve();
  public:
    void setRawData(double* x, double* y, unsigned n);
    signals:
    void setRaw();
                public slots:
                void rawSet();
  private:
    double*  _rawx;
    double*  _rawy;
    unsigned _rawn;
  };
};

#endif
