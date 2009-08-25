#ifndef Pds_MonROOTWaveform_HH
#define Pds_MonROOTWaveform_HH

#include "qwt_plot_curve.h"

#include "pds/mon/MonStats1D.hh"
#include "MonQtBase.hh"

class QwtPlot;

namespace Pds {

  class MonDescWaveform;
  class MonDescTH2F;
  class MonDescImage;
  class MonEntryWaveform;

  class MonQtWaveform : public QwtPlotCurve, 
		    public MonStats1D,
		    public MonQtBase 
  {
  public:
    MonQtWaveform(const char* name, const MonDescWaveform& desc);
    MonQtWaveform(const char* name, const MonDescTH2F& desc, Axis ax);
    MonQtWaveform(const char* name, const MonDescImage& desc, Axis ax);
    virtual ~MonQtWaveform();

    void reset();
    void params(const MonDescWaveform& desc);
    void params(const MonDescTH2F& desc);
    void params(const MonDescImage& desc);
    void setto(const MonEntryWaveform& entry);
    void setto(const MonEntryWaveform& curr, const MonEntryWaveform& prev);
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

  inline void MonQtWaveform::addcontent(unsigned bin, float y)
  {
    *(_y+bin) += y;
  }
};

#endif
