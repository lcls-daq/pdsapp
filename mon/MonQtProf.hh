#ifndef Pds_MonROOTPROF_HH
#define Pds_MonROOTPROF_HH

#include "MonQtBase.hh"

class QwtPlot;
class QwtPlotCurve;

namespace Pds {

  class MonDescProf;
  class MonEntryProf;

  class MonQtProf : public MonQtBase {
  public:
    MonQtProf(const char* name, const MonDescProf& desc);
    virtual ~MonQtProf();

    void params(const MonDescProf& desc);
    void setto(const MonEntryProf& entry);
    void setto(const MonEntryProf& curr, const MonEntryProf& prev);

    // Implements MonQtBase
    virtual void settings(Axis, float vmin, float vmax,
			  bool autorng, bool islog);
    virtual float min(Axis ax) const;
    virtual float max(Axis ax) const;
    virtual void dump(FILE*) const;

    void color(int color);
    int  color() const;

    const double* GetBinContents();
    void SetBins(unsigned nb, double xlo, double xhi);

    void attach(QwtPlot*);

  private:
    void params(const char* names);

  private:
    QwtPlotCurve* _curve;
    double* _x;
    double* _y;
    double* _e;
    double* _y0;
    double* _y1;
    double* _y2;
    unsigned _nbins;
    const char** _names;
    float _xmin, _xmax;
    float _ymin, _ymax;
  };

  inline const double* MonQtProf::GetBinContents() { return _y; }
};

#endif
