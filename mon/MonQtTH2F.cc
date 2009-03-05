#include "MonQtTH2F.hh"
#include "MonQtTH1F.hh"
#include "MonRasterData.hh"

#include "pds/mon/MonEntryTH2F.hh"
#include "pds/mon/MonDescTH2F.hh"

#include "qwt_color_map.h"
#include "qwt_scale_widget.h"
#include "qwt_double_rect.h"
#include "qwt_plot.h"
#include "qwt_scale_engine.h"

namespace Pds {
  class LogColorMap : public QwtColorMap {
  public:
    LogColorMap() :
      _map(Qt::red,Qt::blue)
    {
      _map.addColorStop(0.5,Qt::green);
    }
    LogColorMap(const LogColorMap& m) :
      _map(m._map)
    {
    }
  public:
    virtual QwtColorMap* copy() const { return new LogColorMap(*this); }
    virtual QRgb rgb(const QwtDoubleInterval &ival, 
		     double value) const
    {
      QwtDoubleInterval interval(log(ival.minValue()),
				 log(ival.maxValue()));
      return _map.rgb(interval,log(value));
    }
    virtual unsigned char colorIndex(const QwtDoubleInterval &ival, 
				     double value) const
    {
      QwtDoubleInterval interval(log(ival.minValue()),
				 log(ival.maxValue()));
      return _map.colorIndex(interval,log(value));
    }

  private:
    QwtLinearColorMap _map;
  };
};

using namespace Pds;

MonQtTH2F::MonQtTH2F(const char* name, 
		     const MonDescTH2F& desc) : 
  QwtPlotSpectrogram(name),
  MonQtBase(H2, desc, name),
  _data(new MonRasterData(desc.nbinsx(),desc.nbinsy(),
			  desc.xlow(), desc.xup(),
			  desc.ylow(), desc.yup(),
			  _zmin, _zmax))
{
  params(desc);
  settings(MonQtBase::X, desc.xlow(), desc.xup(), true, false);
  settings(MonQtBase::Y, desc.ylow(), desc.yup(), true, false);
  settings(MonQtBase::Z, 0, 1, true, false);
  //  Assign the z-axis
  QwtLinearColorMap colorMap(Qt::red, Qt::blue);
  colorMap.addColorStop(0.5,Qt::green);
  setColorMap(colorMap);
}

MonQtTH2F::~MonQtTH2F() {}

void MonQtTH2F::setto(const MonEntryTH2F& entry) 
{
  const MonDescTH2F& d(reinterpret_cast<const MonDescTH2F&>(*_desc));
  last(entry.last());

  float* dst = _data->data();
  float* last = dst + d.nbinsx()*d.nbinsy();
  unsigned bin = 0;
  if (entry.desc().isnormalized()) {
    float norm = entry.info(MonEntryTH2F::Normalization);
    float scale = norm > 0 ? 1.0/norm : 1;
    while (dst < last) {
      *dst++ = entry.content(bin++)*scale;
    }
  } else {
    while (dst < last) {
      *dst++ = entry.content(bin++);
    }
    //    _data->setScale(0,double(entry.desc().nentries()));
  }
  setData(*_data);
}

void MonQtTH2F::setto(const MonEntryTH2F& curr, 
		      const MonEntryTH2F& prev) 
{
  const MonDescTH2F& d(reinterpret_cast<const MonDescTH2F&>(*_desc));
  last(curr.last());
  float* dst  = _data->data();
  float* last = dst + d.nbinsx()*d.nbinsy();

  unsigned bin = 0;
  if (curr.desc().isnormalized()) {
    float dnorm = curr.info(MonEntryTH2F::Normalization)-prev.info(MonEntryTH2F::Normalization);
    float scale = dnorm > 0 ? 1.0/dnorm : 1;
    while (dst < last) {
      *dst++ = (curr.content(bin) - prev.content(bin)) * scale;
      bin++;
    }
  } else {
    while (dst < last) {
      *dst++ = curr.content(bin) - prev.content(bin);
      bin++;
    }
    //    _data->setScale(0,double(curr.desc().nentries()-
    // 			     prev.desc().nentries()));
  }
  setData(*_data);
}

void MonQtTH2F::stats() 
{
  const MonDescTH2F& d(reinterpret_cast<const MonDescTH2F&>(*_desc));
  MonStats2D::stats(d.nbinsx(), d.nbinsy(),
		    d.xlow(), d.xup(),
		    d.ylow(), d.yup(),
		    _data->data());
  _stats[0] = meanx();
  _stats[1] = rmsx();
  _stats[2] = meany();
  _stats[3] = rmsy();
}

void MonQtTH2F::projectx(MonQtTH1F* h) 
{
  h->reset();
  h->last(last());
  const MonDescTH2F& d(reinterpret_cast<const MonDescTH2F&>(*_desc));
  int nx = d.nbinsx();
  int ny = d.nbinsy();
  float* z = _data->data();
  for (int ybin=0; ybin<ny; ybin++) {
    for (int xbin=0; xbin<nx; xbin++) {
      h->addcontent(xbin, *z++);
    }
  }
}

void MonQtTH2F::projecty(MonQtTH1F* h) 
{
  h->reset();
  h->last(last());
  const MonDescTH2F& d(reinterpret_cast<const MonDescTH2F&>(*_desc));
  int nx = d.nbinsx();
  int ny = d.nbinsy();
  float* z = _data->data();
  for (int ybin=0; ybin<ny; ybin++) {
    for (int xbin=0; xbin<nx; xbin++) {
      h->addcontent(ybin, *z++);
    }
  }
}

float MonQtTH2F::min(Axis ax) const 
{
  return (ax == X) ? _xmin : (ax==Y) ? _ymin : _zmin;
}

float MonQtTH2F::max(Axis ax) const 
{
  return (ax == X) ? _xmax : (ax==Y) ? _ymax : _zmax;
}

void MonQtTH2F::color(int color)
{
}

int  MonQtTH2F::color() const
{
  return 0;
}

void MonQtTH2F::attach(QwtPlot* plot)
{
  if (!plot) {
    QwtScaleWidget *zAxis = _plot->axisWidget(QwtPlot::xTop);
    zAxis->setColorBarEnabled(false);
  }

  MonQtBase::attach(plot);
  QwtPlotSpectrogram::attach(plot);

  if (plot) {

//     plot->setAxisScale(QwtPlot::xTop,
// 		       data().range().minValue(),
// 		       data().range().maxValue() );
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

    if (isautorng(MonQtBase::Z))
      plot->setAxisAutoScale(QwtPlot::xTop);
    else
      plot->setAxisScale(QwtPlot::xTop, _zmin, _zmax);
    if (islog(MonQtBase::Z)) {
      plot->setAxisScaleEngine(QwtPlot::xTop, new QwtLog10ScaleEngine);
      LogColorMap map;
      setColorMap(map);
    }
    else {
      plot->setAxisScaleEngine(QwtPlot::xTop, new QwtLinearScaleEngine);
      QwtLinearColorMap colorMap(Qt::red, Qt::blue);
      colorMap.addColorStop(0.5,Qt::green);
      setColorMap(colorMap);
    }
    QwtScaleWidget *zAxis = plot->axisWidget(QwtPlot::xTop);
    zAxis->setColorBarEnabled(true);
    zAxis->setColorMap(data().range(),colorMap());
    plot->enableAxis(QwtPlot::xTop);
  }
}

void MonQtTH2F::settings(MonQtBase::Axis ax, float vmin, float vmax,
			 bool autorng, bool islog)
{
  switch(ax) {
  case MonQtBase::X: _xmin=vmin; _xmax=vmax; break;
  case MonQtBase::Y: _ymin=vmin; _ymax=vmax; break;
  case MonQtBase::Z: _zmin=vmin; _zmax=vmax; break;
  default: break;
  }
  MonQtBase::settings(ax,vmin,vmax,autorng,islog);
}
