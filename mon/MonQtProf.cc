#include "MonQtProf.hh"
#include "MonPath.hh"
#include "MonUtils.hh"

#include "pds/mon/MonEntryProf.hh"

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_scale_engine.h"

#include <QtGui/QColor>
#include <QtGui/QPen>

using namespace Pds;

MonQtProf::MonQtProf(const char* name, 
		     const MonDescProf& desc) : 
  MonQtBase(Prof, desc, name),
  _nbins(0),
  _names(0)
{ 
  _curve = new QwtPlotCurve(name);
  _curve->setStyle(QwtPlotCurve::Steps);
  _curve->setPen  (QPen(MonUtils::color(0)));
  params(desc);
  settings(MonQtBase::X, desc.xlow(), desc.xup(), true, false);
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtProf::~MonQtProf() 
{
  if (_nbins) {
    delete[] _x;
    delete[] _y;
    delete[] _e;
    delete[] _y0;
    delete[] _y1;
    delete[] _y2;
  }
  if (_names) delete _names;
}

void MonQtProf::params(const char* names) 
{
  if (_names) delete _names;
  _names = new const char*[_nbins];
  unsigned found = MonPath::split(names, _names, _nbins);
  if (found != _nbins) {
    delete _names;
    _names = 0;
  }
}

void MonQtProf::params(const MonDescProf& desc) 
{
  MonQtBase::params(desc);
  SetBins(desc.nbins(), desc.xlow(), desc.xup());
  params(desc.names());
}

void MonQtProf::SetBins(unsigned nb, double xlo, double xhi)
{
  if (_nbins) {
    delete[] _x;
    delete[] _y;
    delete[] _e;
    delete[] _y0;
    delete[] _y1;
    delete[] _y2;
  }
  _nbins = nb;
  if (nb) {
    _x  = new double[nb+1];
    _y  = new double[nb+1];
    _e  = new double[nb+1];
    _y0 = new double[nb];
    _y1 = new double[nb];
    _y2 = new double[nb];
    memset(_y ,0,(nb+1)*sizeof(double));
    memset(_e ,0,(nb+1)*sizeof(double));
    memset(_y0,0,nb*sizeof(double));
    memset(_y1,0,nb*sizeof(double));
    memset(_y2,0,nb*sizeof(double));
    double dx = (xhi-xlo)/double(nb);
    for(unsigned k=0; k<=nb; k++)
      _x[k] = xlo + dx*double(k);
    _curve->setRawData(_x, _y, _nbins+1); // QwtPlotCurve wants the x-endpoint
    _curve->setStyle(QwtPlotCurve::Steps);
    _curve->setPen  (QPen(MonUtils::color(0)));
    //  Qwt claims to plot steps left to right if Inverted is false,
    //  but I find the opposite.  I note that the source reverses direction
    //  for Yfx (Xfy) plots, also.
    _curve->setCurveAttribute(QwtPlotCurve::Inverted,true);
  }
}

void MonQtProf::setto(const MonEntryProf& entry) 
{
  last(entry.last());
  double* y  = _y;
  double* y0 = _y0;
  double* y1 = _y1;
  double* y2 = _y2;

  for(unsigned bin = 0; bin<_nbins; bin++) {
    *y0   = entry.nentries(bin);
    *y1   = entry.ysum(bin);
    *y++  = (*y0) ? *y1 / *y0 : 0;
    *y2++ = entry.y2sum(bin);
    y0++;
    y1++;
  }
}

void MonQtProf::setto(const MonEntryProf& curr, 
		      const MonEntryProf& prev) 
{
  last(curr.last());
  double* y  = _y;
  double* y0 = _y0;
  double* y1 = _y1;
  double* y2 = _y2;

  for(unsigned bin = 0; bin<_nbins; bin++) {
    *y0   = curr.nentries(bin)-prev.nentries(bin);
    *y1   = curr.ysum    (bin)-prev.ysum    (bin);
    *y++  = (*y0) ? *y1 / *y0 : 0;
    *y2++ = curr.y2sum   (bin)-prev.y2sum   (bin);
    y0++;
    y1++;
  }
}

float MonQtProf::min(Axis ax) const 
{
  return (ax == X) ? _xmin : (ax==Y) ? _ymin : 0;
}

float MonQtProf::max(Axis ax) const 
{
  return (ax == X) ? _xmax : (ax==Y) ? _ymax : 0;
}

void MonQtProf::dump(FILE* f) const
{
  int nb = _nbins;
  for(int i=0; i<nb; i++)
    fprintf(f,"%g %g\n",0.5*(_x[i]+_x[i+1]),_y[i]);
}

void MonQtProf::color(int color)
{
}

int  MonQtProf::color() const
{
  return 0;
}

void MonQtProf::attach(QwtPlot* plot)
{
  MonQtBase::attach(plot);
  _curve->attach(plot);
  if (plot) {
    if (isautorng(MonQtBase::X))
      plot->setAxisAutoScale(QwtPlot::xBottom);
    else
      plot->setAxisScale(QwtPlot::xBottom, _xmin, _xmax);
    if (isautorng(MonQtBase::Y))
      plot->setAxisAutoScale(QwtPlot::yLeft);
    else
      plot->setAxisScale(QwtPlot::yLeft, _ymin, _ymax);
    if (islog(MonQtBase::Y))
      plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine);
    else
      plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
  }
}

void MonQtProf::settings(MonQtBase::Axis ax, float vmin, float vmax,
			 bool autorng, bool islog)
{
  switch(ax) {
  case MonQtBase::X: _xmin=vmin; _xmax=vmax; break;
  case MonQtBase::Y: _ymin=vmin; _ymax=vmax; break;
  default: break;
  }
  MonQtBase::settings(ax,vmin,vmax,autorng,islog);
}
