#ifndef Pds_MonROOTTH2F_HH
#define Pds_MonROOTTH2F_HH

#include "qwt_plot_spectrogram.h"

#include "pds/mon/MonStats2D.hh"
#include "MonQtBase.hh"

class QwtRasterData;
class QwtPlot;

namespace Pds {

  class MonQtTH1F;
  class MonDescTH2F;
  class MonEntryTH2F;
  class MonRasterData;

  class MonQtTH2F : public QwtPlotSpectrogram,
		    public MonStats2D, 
		    public MonQtBase
  {
  public:
    MonQtTH2F(const char* name, const MonDescTH2F& desc);
    virtual ~MonQtTH2F();

    //    void params(const MonDescTH2F& desc);
    void setto(const MonEntryTH2F& entry);
    void setto(const MonEntryTH2F& curr, const MonEntryTH2F& prev);
    void stats();

    void projectx(MonQtTH1F* h);
    void projecty(MonQtTH1F* h);

    void attach(QwtPlot*);

    // Implements MonQtBase
    virtual void settings(Axis, float vmin, float vmax,
			  bool autorng, bool islog);
    virtual float min(Axis ax) const;
    virtual float max(Axis ax) const;
    virtual void dump(FILE*) const;

    void color(int color);
    int  color() const;

    void SetBins(unsigned nx, double xlo, double xhi,
		 unsigned ny, double ylo, double yhi);
  private:
    double _stats[4];
    MonRasterData* _data;
    float _xmin, _xmax;
    float _ymin, _ymax;
    float _zmin, _zmax;
  };
};
#endif
