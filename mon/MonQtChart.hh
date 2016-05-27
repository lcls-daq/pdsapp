#ifndef Pds_MonROOTCHART_HH
#define Pds_MonROOTCHART_HH

#include "MonQtBase.hh"
#include <QtGui/QColor>
#include <QtCore/QString>

#include <vector>

class QwtPlot;

namespace Pds {

  class MonDescProf;
  class MonDescScalar;
  class MonDescTH1F;
  class MonDescTH2F;
  class MonDescImage;
  class QtPlotCurve;

  class MonQtChart : public MonQtBase {
  public:
    MonQtChart(const char* name, const MonDescScalar& desc);
    MonQtChart(const char* name, const MonDescProf& desc);
    MonQtChart(const char* name, const MonDescTH1F& desc);
    MonQtChart(const char* name, const MonDescTH2F& desc, Axis ax);
    MonQtChart(const char* name, const MonDescImage& desc, Axis ax);
    virtual ~MonQtChart();

    void params(const MonDescScalar& desc);
    void params(const MonDescProf& desc);
    void params(const MonDescTH1F& desc);
    void params(const MonDescTH2F& desc);
    void params(const MonDescImage& desc);

    // Set number of points
    void points(unsigned np);

    // Set values for a point for all lines
    void point(double time, const double* y);
    void point(double time, const std::vector<double>&);

    // Set a value for a point for first line
    void point(double time, double y);

    // Implements MonQtBase
    virtual void settings(Axis, float vmin, float vmax,
			  bool autorng, bool islog);
    virtual float min(Axis ax) const;
    virtual float max(Axis ax) const;
    virtual void dump(FILE*) const;

    void color(int color);
    int  color() const;

    void attach(QwtPlot*);

    void dump() const;

    unsigned    nlines()           const { return _nlines; }
    QString     name  (unsigned i) const;
    QColor      qcolor(unsigned i) const;
  private:
    void params(unsigned, const char* names);
    void autorange();

  private:
    unsigned _npoints;
    unsigned _nfill;
    unsigned _nlines;
    unsigned _current;

    double* _xl;
    double* _yl;

    QtPlotCurve** _curves;

    float _ymin, _ymax;
  };
};

#endif


  
