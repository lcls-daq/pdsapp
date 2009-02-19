#ifndef Pds_MonROOTCHART_HH
#define Pds_MonROOTCHART_HH

#include "MonQtBase.hh"

class QwtPlot;
class QwtPlotCurve;

namespace Pds {

  class MonDescProf;
  class MonDescTH1F;
  class MonDescTH2F;
  class MonDescImage;

  class MonQtChart : public MonQtBase {
  public:
    MonQtChart(const char* name, const MonDescProf& desc);
    MonQtChart(const char* name, const MonDescTH1F& desc);
    MonQtChart(const char* name, const MonDescTH2F& desc, Axis ax);
    MonQtChart(const char* name, const MonDescImage& desc, Axis ax);
    virtual ~MonQtChart();

    void params(const MonDescProf& desc);
    void params(const MonDescTH1F& desc);
    void params(const MonDescTH2F& desc);
    void params(const MonDescImage& desc);

    // Set number of points
    void points(unsigned np);

    // Set values for a point for all lines
    void point(double time, const double* y);

    // Set a value for a point for first line
    void point(double time, double y);

    // Implements MonQtBase
    virtual void settings(Axis, float vmin, float vmax,
			  bool autorng, bool islog);
    virtual float min(Axis ax) const;
    virtual float max(Axis ax) const;

    void color(int color);
    int  color() const;

    void attach(QwtPlot*);

  private:
    void params(unsigned, const char* names);
    void autorange();

  private:
    unsigned _npoints;
    unsigned _nlines;
    unsigned _current;

    double* _xl;
    double* _yl;

    QwtPlotCurve** _curves;
    const char** _names;

    float _ymin, _ymax;
  };
};

#endif


  
