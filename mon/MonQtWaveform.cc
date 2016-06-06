#include "MonQtWaveform.hh"
#include "MonUtils.hh"

#include "pds/mon/MonEntryWaveform.hh"
#include "pds/mon/MonDescTH2F.hh"
#include "pds/mon/MonDescImage.hh"

#include <QtGui/QBrush>
#include <QtGui/QPen>

#include "qwt_plot.h"
#include "qwt_scale_engine.h"

using namespace Pds;

#define NBINS (dataSize()-1)

MonQtWaveform::MonQtWaveform(const char* name, 
			     const MonDescWaveform& desc) : 
  QtPlotCurve(name),
  MonQtBase(H1, desc, name)
{
  setStyle(QwtPlotCurve::Steps);
  setPen  (QPen(MonUtils::color(0)));
  //  Qwt has a problem with logY plots when using the brush to fill
  //  setBrush(QBrush(MonUtils::color(0)));

  //  Qwt claims to plot steps left to right if Inverted is false,
  //  but I find the opposite.  I note that the source reverses direction
  //  for Yfx (Xfy) plots, also.
  setCurveAttribute(QwtPlotCurve::Inverted,true);

  SetBins(desc.nbins(), desc.xlow(), desc.xup());
  settings(MonQtBase::X, desc.xlow(), desc.xup(), true, false);
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtWaveform::MonQtWaveform(const char* name, 
			     const MonDescTH2F& desc,
			     Axis ax) : 
  QtPlotCurve(name),
  MonQtBase(H1, desc, name, ax == X ? false : true)
{
  setStyle(QwtPlotCurve::Steps);
  setPen  (QPen(MonUtils::color(0)));
  //  setBrush(QBrush(MonUtils::color(0)));
  setCurveAttribute(QwtPlotCurve::Inverted,true);

  if (ax==X) {
    SetBins(desc.nbinsx(), desc.xlow(), desc.xup());
    settings(MonQtBase::X, desc.xlow(), desc.xup(), true, false);
  }
  else {
    SetBins(desc.nbinsy(), desc.ylow(), desc.yup());
    settings(MonQtBase::X, desc.ylow(), desc.yup(), true, false);
  }
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtWaveform::MonQtWaveform(const char* name, 
			     const MonDescImage& desc,
			     Axis ax) : 
  QtPlotCurve(name),
  MonQtBase(H1, desc, name, ax == X ? false : true)
{
  setStyle(QwtPlotCurve::Steps);
  setPen  (QPen(MonUtils::color(0)));
  //  setBrush(QBrush(MonUtils::color(0)));
  setCurveAttribute(QwtPlotCurve::Inverted,true);

  if (ax==X) {
    SetBins(desc.nbinsx(), desc.xlow(), desc.xup());
    settings(MonQtBase::X, desc.xlow(), desc.xup(), true, false);
  }
  else {
    SetBins(desc.nbinsy(), desc.ylow(), desc.yup());
    settings(MonQtBase::X, desc.ylow(), desc.yup(), true, false);
  }
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtWaveform::~MonQtWaveform() 
{
  delete[] _x;
  delete[] _y;
}

void MonQtWaveform::SetBins(unsigned nb, double xlow, double xhi)
{
  _x = new double[nb+1];
  _y = new double[nb+1];
  double dx = nb ? (xhi - xlow) / double(nb) : 0;
  double x0 = xlow;
  for(unsigned k=0; k<=nb; k++)
    _x[k] = x0 + double(k)*dx;
  setRawData(_x,_y,nb+1);  // QwtPlotCurve wants the x-endpoint
  reset();
}

void MonQtWaveform::reset() 
{
  memset(_y, 0, sizeof(double)*(NBINS+1));
}

void MonQtWaveform::params(const MonDescWaveform& desc) 
{
  MonQtBase::params(desc);
  if (NBINS != (int)desc.nbins()) {
    delete[] _x;
    delete[] _y;
    SetBins(desc.nbins(), desc.xlow(), desc.xup());
  }
}

void MonQtWaveform::params(const MonDescTH2F& desc) 
{
  MonQtBase::params(desc);
  SetBins(!swapaxis() ? desc.nbinsx() : desc.nbinsy(), 
	  !swapaxis() ? desc.xlow() : desc.ylow(),
	  !swapaxis() ? desc.xup()  : desc.yup());
}

void MonQtWaveform::params(const MonDescImage& desc) 
{
  MonQtBase::params(desc);
  SetBins(!swapaxis() ? desc.nbinsx() : desc.nbinsy(), 
	  !swapaxis() ? desc.xlow() : desc.ylow(),
	  !swapaxis() ? desc.xup()  : desc.yup());
}

void MonQtWaveform::setto(const MonEntryWaveform& entry) 
{
  last(entry.last());
  double* dst  = _y;
  double* last = _y + NBINS;

  unsigned bin = 0;
  if (entry.desc().isnormalized()) {
    double norm = entry.info(MonEntryWaveform::Normalization);
    double scale = norm > 0 ? 1.0/norm : 1;
    while (dst < last) {
      *dst++ = entry.content(bin++)*scale;
    }
  } else {
    while (dst < last) {
      *dst++ = entry.content(bin++);
    }
  }
}

void MonQtWaveform::setto(const MonEntryWaveform& curr, 
			  const MonEntryWaveform& prev) 
{
  last(curr.last());
  double* dst  = _y;
  double* last = _y + NBINS;

  unsigned bin = 0;
  if (curr.desc().isnormalized()) {
    double dnorm = curr.info(MonEntryWaveform::Normalization)-prev.info(MonEntryWaveform::Normalization);
    double scale = dnorm > 0 ? 1.0/dnorm : 1;
    while (dst < last) {
      *dst++ = (curr.content(bin) - prev.content(bin)) * scale;
      bin++;
    }
  } else {
    while (dst < last) {
      *dst++ = curr.content(bin) - prev.content(bin);
      bin++;
    }
  }
}

void MonQtWaveform::stats() 
{
  int nb = NBINS;
  MonStats1D::stats(nb, _x[0], _x[nb], _y);

  _stats[0] = mean();
  _stats[1] = rms();
  _stats[2] = sum();
  _stats[3] = under();
  _stats[4] = over();
}

float MonQtWaveform::min(Axis ax) const 
{
  return (ax == X) ? _xmin : (ax==Y) ? _ymin : 0;
}

float MonQtWaveform::max(Axis ax) const 
{
  return (ax == X) ? _xmax : (ax==Y) ? _ymax : 0;
}

void MonQtWaveform::dump(FILE* f) const
{
  int nb = NBINS;
  for(int i=0; i<nb; i++)
    fprintf(f,"%g %g\n",0.5*(_x[i]+_x[i+1]),_y[i]);
}

void MonQtWaveform::color(int color)
{
  //  SetLineColor(color);
}

int  MonQtWaveform::color() const
{
  //  return GetLineColor();
  return 0;
}

void MonQtWaveform::attach(QwtPlot* plot)
{
  MonQtBase::attach(plot);
  QwtPlotCurve::attach(plot);
  if (plot) {
    if (isautorng(MonQtBase::X))
      plot->setAxisAutoScale(QwtPlot::xBottom);
    else
      plot->setAxisScale(QwtPlot::xBottom, _xmin, _xmax);
    if (islog(MonQtBase::Y))
      plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine);
    else
      plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
    if (isautorng(MonQtBase::Y))
      plot->setAxisAutoScale(QwtPlot::yLeft);
    else
      plot->setAxisScale(QwtPlot::yLeft, _ymin, _ymax, 0.2*(_ymax-_ymin));
  }
  printf("plot %p attached\n",plot);
}

void MonQtWaveform::settings(MonQtBase::Axis ax, float vmin, float vmax,
			     bool autorng, bool islog)
{
  switch(ax) {
  case MonQtBase::X: _xmin=vmin; _xmax=vmax; break;
  case MonQtBase::Y: _ymin=vmin; _ymax=vmax; break;
  default: break;
  }
  MonQtBase::settings(ax,vmin,vmax,autorng,islog);
}
