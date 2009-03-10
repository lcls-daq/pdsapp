#include "MonQtChart.hh"
#include "MonPath.hh"
#include "MonUtils.hh"
#include "pds/mon/MonDescProf.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonDescTH2F.hh"
#include "pds/mon/MonDescImage.hh"

#include <QtGui/QBrush>
#include <QtGui/QPen>
#include "qwt_plot_curve.h"
#include "qwt_plot.h"
#include "qwt_scale_engine.h"

static const unsigned Points = 64;
static const unsigned MaxColors = 8;
static const unsigned MaxDisplayNames = 16;
static const unsigned MaxLines = 32;

using namespace Pds;

MonQtChart::MonQtChart(const char* name, 
		       const MonDescProf& desc) :
  MonQtBase(Chart, desc, name),
  _npoints(Points),
  _nlines(0),
  _current(0),
  _xl(0),
  _yl(0),
  _curves(0)
{
  params(desc.nbins(),desc.names());
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtChart::MonQtChart(const char* name, 
		       const MonDescTH1F& desc) : 
  MonQtBase(Chart, desc, name),
  _npoints(Points),
  _nlines(1),
  _current(0),
  _xl(new double[2*_npoints]),
  _yl(new double[2*_npoints]),
  _curves(0)
{
  params(1,name);
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtChart::MonQtChart(const char* name, 
		       const MonDescTH2F& desc,
		       Axis ax) : 
  MonQtBase(Chart, desc, name, ax == X ? true : false),
  _npoints(Points),
  _nlines(1),
  _current(0),
  _xl(new double[2*_npoints]),
  _yl(new double[2*_npoints]),
  _curves(0)
{
  params(1,name);
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtChart::MonQtChart(const char* name, 
		       const MonDescImage& desc,
		       Axis ax) : 
  MonQtBase(Chart, desc, name, ax == X ? true : false),
  _npoints(Points),
  _nlines(1),
  _current(0),
  _xl(new double[2*_npoints]),
  _yl(new double[2*_npoints]),
  _curves(0)
{
  params(1,name);
  settings(MonQtBase::Y, 0, 1, true, false);
}

MonQtChart::~MonQtChart() 
{
  for(unsigned k=0; k<_nlines; k++)
    delete _curves[k];
  delete [] _curves;
  delete [] _xl;
  delete [] _yl;
}

void MonQtChart::params(unsigned nl, const char* names)
{
  unsigned m = 2*_npoints;
  if (nl != _nlines) {
    if (_xl) delete[] _xl;
    if (_yl) delete[] _yl;
    // Create arrays with requested length
    _xl = new double[m];
    _yl = new double[m*nl];
  }
  
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME,&tv);
  for(unsigned k=0; k<m; k++)
    _xl[k] = tv.tv_sec;

  memset(_yl, 0, m*nl*sizeof(double));

  // create the curves
  if (_curves) {
    for(unsigned k=0; k<_nlines; k++) {
      _curves[k]->attach(NULL);
      delete _curves[k];
    }
    delete[] _curves;
  }
  
  _nlines = nl;
  _curves = new QwtPlotCurve*[_nlines];
    
  const char** snames = new const char*[_nlines];
  unsigned nfound = MonPath::split(names, snames, _nlines);
  for(unsigned k=0; k<_nlines; k++) {
    char nameb[64];
    sprintf(nameb,"%s:%d",_name,k);
    QwtPlotCurve* c = new QwtPlotCurve((nfound==_nlines) ? snames[k] : nameb);
    //  c->setStyle(QwtPlotCurve::Dots);
    c->setStyle(QwtPlotCurve::Steps);
    c->setPen  (QPen(MonUtils::color(k,_nlines)));
    _curves[k] = c;
  }
  delete[] snames;
}

void MonQtChart::params(const MonDescProf& desc)
{
  params(desc.nbins(),desc.names());
  points(_npoints);
}

void MonQtChart::params(const MonDescTH1F& desc)
{
  MonQtBase::params(desc);
}

void MonQtChart::params(const MonDescTH2F& desc)
{
  MonQtBase::params(desc);
}

void MonQtChart::params(const MonDescImage& desc)
{
  MonQtBase::params(desc);
}

void MonQtChart::points(unsigned np) 
{
  if (np != _npoints) {
    // Create arrays with requested length
    unsigned m = 2*np;
    double* tempx = new double[m];
    double* tempy = new double[m*_nlines];

    // Copy arrays
    unsigned stp = _npoints*2;
    unsigned src = (np < _npoints) ? _npoints+_current-np : _current;
    unsigned dst = 0;
    unsigned len = (np < _npoints) ? np : _npoints;
    memcpy(&tempx[dst+0 ], &_xl[src], len*sizeof(double));
    memcpy(&tempx[dst+np], &_xl[src], len*sizeof(double));
    for(unsigned k=0; k<_nlines; k++, dst+=m, src+=stp) {
      memcpy(&tempy[dst+0 ], &_yl[src], len*sizeof(double));
      memcpy(&tempy[dst+np], &_yl[src], len*sizeof(double));
    }
    if (np > _npoints)  // fill in the extra history
      for(unsigned j=_npoints; j<np; j++) {
	src = _current;
	tempx[j+0 ] = _xl[src];
	tempx[j+np] = _xl[src];
	dst = j;
	for(unsigned k=0; k<_nlines; k++, dst+=m, src+=stp) {
	  tempy[dst+0 ] = _yl[src];
	  tempy[dst+np] = _yl[src];
	}
      }

    _current = (np < _npoints) ? 0 : _current;
    _npoints = np;

    // Delete old arrays
    delete [] _xl;
    delete [] _yl;
    _xl = tempx;
    _yl = tempy;
  }
}

void MonQtChart::point(double time, const double* y) 
{
  last(time);
  _xl[_current+0       ] = time;
  _xl[_current+_npoints] = time;

  double* dst = _yl+_current;
  const double* end = y+_nlines;
  do {
    *dst = *y;
    dst += _npoints;
    *dst = *y;
    dst += _npoints;
  } while (++y < end);

  if (++_current >= _npoints)
    _current = 0;

  double* yl = &_yl[_current];
  for(unsigned k=0; k<_nlines; k++) {
    _curves[k]->setRawData(&_xl[_current], yl, _npoints);
    yl += 2*_npoints;
  }
}

void MonQtChart::point(double time, double y) 
{
  last(time);
  _xl[_current         ] = time;
  _xl[_current+_npoints] = time;
  _yl[_current         ] = y;
  _yl[_current+_npoints] = y;
  if (++_current >= _npoints)
    _current = 0;
  _curves[0]->setRawData(&_xl[_current], &_yl[_current], _npoints);
}

float MonQtChart::min(Axis ax) const 
{
  return (ax == X) ? 0 : (ax==Y) ? _ymin : 0;
}

float MonQtChart::max(Axis ax) const 
{
  return (ax == X) ? float(_npoints) : (ax==Y) ? _ymax : 0;
}

void MonQtChart::color(int color)
{
}

int  MonQtChart::color() const
{
  return 0;
}

void MonQtChart::attach(QwtPlot* plot)
{
  MonQtBase::attach(plot);
  for(unsigned k=0; k<_nlines; k++)
    _curves[k]->attach(plot);
  if (plot) {
    plot->setAxisAutoScale(QwtPlot::xBottom);
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

void MonQtChart::settings(MonQtBase::Axis ax, float vmin, float vmax,
			  bool autorng, bool islog)
{
  switch(ax) {
  case MonQtBase::X: if (vmax>=1) points(int(vmax)); break;
  case MonQtBase::Y: _ymin=vmin; _ymax=vmax; break;
  default: break;
  }
  MonQtBase::settings(ax,vmin,vmax,autorng,islog);
}

