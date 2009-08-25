#ifndef Pds_MonROOTTH1F_HH
#define Pds_MonROOTTH1F_HH

#include "qwt_plot_curve.h"

#include "pds/mon/MonStats1D.hh"
#include "MonQtBase.hh"

class QwtPlot;

namespace Pds {

  class MonDescTH1F;
  class MonDescTH2F;
  class MonDescImage;
  class MonEntryTH1F;

  class MonQtTH1F : public QwtPlotCurve, 
		    public MonStats1D,
		    public MonQtBase 
  {
  public:
    MonQtTH1F(const char* name, const MonDescTH1F& desc);
    MonQtTH1F(const char* name, const MonDescTH2F& desc, Axis ax);
    MonQtTH1F(const char* name, const MonDescImage& desc, Axis ax);
    virtual ~MonQtTH1F();

    void reset();
    void params(const MonDescTH1F& desc);
    void params(const MonDescTH2F& desc);
    void params(const MonDescImage& desc);
    void setto(const MonEntryTH1F& entry);
    void setto(const MonEntryTH1F& curr, const MonEntryTH1F& prev);
    void stats();

    void attach(QwtPlot*);

    // Implements MonQtBase
    virtual void settings(Axis, float vmin, float vmax,
			  bool autorng, bool islog);
    virtual float min(Axis ax) const;
    virtual float max(Axis ax) const;
    virtual void dump(FILE*) const;

    void color(int color);
    int  color() const;

    void SetBins(unsigned, double, double);
    void addcontent(unsigned bin, float y);
  private:
    double* _x;
    double* _y;
    double _stats[5];
    float _xmin, _xmax;
    float _ymin, _ymax;
  };

  inline void MonQtTH1F::addcontent(unsigned bin, float y)
  {
    *(_y+bin) += y;
  }
};

#endif
