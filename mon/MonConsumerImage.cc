#include <string.h>

#include "MonConsumerImage.hh"
#include "MonQtTH1F.hh"
#include "MonQtImage.hh"
#include "MonQtChart.hh"
#include "MonDialog.hh"
#include "MonQtImageDisplay.hh"
#include "MonTimeScale.hh"

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryImage.hh"
#include "pds/mon/MonDescImage.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QStackedWidget>

#include "qwt_plot.h"

using namespace Pds;

enum { Image, Plots };

MonConsumerImage::MonConsumerImage(QWidget& parent,
				 const MonDesc& clientdesc,
				 const MonGroup& group, 
				 const MonEntryImage& entry) :
  MonCanvas(parent, entry),
  _group(group),
  _desc(new MonDescImage(entry.desc())),
  _hist(0),
  _hist_x(0),
  _hist_y(0),
  _chartx(0),
  _charty(0),
  _dialog(0),
  _time  (0,0)
{
  // Initialize histograms
  const MonDescImage& desc = *_desc;
  const char* clientname = clientdesc.name();
  const char* dirname = group.desc().name();
  const char* entryname = desc.name();

  char tmp[128];
  snprintf(tmp, 128, "%s:%s:%s:HIST", clientname, dirname, entryname);
  _hist = new MonQtImage(tmp, desc);

  snprintf(tmp,  128, "%s:%s:%s:HISTX", clientname, dirname, entryname);
  _hist_x = new MonQtTH1F(tmp, desc, MonQtBase::X);

  snprintf(tmp,  128, "%s:%s:%s:HISTY", clientname, dirname, entryname);
  _hist_y = new MonQtTH1F(tmp, desc, MonQtBase::Y);

  snprintf(tmp, 128, "%s:%s:%s:CHARTX", clientname, dirname, entryname);
  _chartx = new MonQtChart(tmp, desc, MonQtBase::X);

  snprintf(tmp, 128, "%s:%s:%s:CHARTY", clientname, dirname, entryname);
  _charty = new MonQtChart(tmp, desc, MonQtBase::Y);

  _frame = new MonQtImageDisplay(NULL);
  _hist->attach(_frame);

  _plot  = new QwtPlot;
  connect( this, SIGNAL(redraw()), _frame , SLOT(display()) );
  connect( this, SIGNAL(redraw()), _plot  , SLOT(replot()) );
  _plot->setAutoDelete(false);

  _stack = new QStackedWidget(this);
  _stack->addWidget(_frame);
  _stack->addWidget(_plot);

  layout()->addWidget(_stack);

  // Prepares menus
  _menu_service(Normal     , true);
  _menu_service(ProjectionX, false);
  _menu_service(ProjectionY, false);
  _menu_service(ChartX     , false);
  _menu_service(ChartY     , false);

  select(Normal);
}

MonConsumerImage::~MonConsumerImage() 
{
  delete _desc;
  delete _hist;
  delete _hist_x;
  delete _hist_y;
  delete _chartx;
  delete _charty;
  if (_dialog) delete _dialog;
}

void MonConsumerImage::dialog()
{
  if (!_dialog)
    _dialog = new MonDialog(this,
                            _hist, 
                            _hist_x, _hist_y,
                            _chartx, _charty);
  _dialog->show();
}

int MonConsumerImage::_update() 
{
  const MonEntryImage* entry = dynamic_cast<const MonEntryImage*>(_entry);
  if (entry->time() > _time) {
    _hist->setto(*entry);
    _hist->projectx(_hist_x);
    _hist->projecty(_hist_y);
    _hist_x->stats();
    _hist_y->stats();
    _chartx->point(entry->last(), _hist_x->mean());
    _charty->point(entry->last(), _hist_y->mean());
    _time = entry->time();
    emit redraw();
    return 1;
  }
  return 0;
}

int MonConsumerImage::_reset()
{
  _entry = _group.entry(_entry->desc().name());
  if (_entry && _entry->desc().type() == MonDescEntry::Image) {
    const MonEntryImage* entry = dynamic_cast<const MonEntryImage*>(_entry);
    *_desc = entry->desc();
    const MonDescImage& desc = *_desc;
    _hist  ->params(desc);
    _hist_x->params(desc);
    _hist_y->params(desc);
    _chartx->params(desc);
    _charty->params(desc);
    return 1;
  }
  _entry = 0;
  return 0;
}

static const unsigned Nplots = 5;
static const char* Names[Nplots] = {
  "Normal",
  "ProjectionX",
  "ProjectionY",
  "StripChartX",
  "StripChartY"
};

unsigned MonConsumerImage::getplots(MonQtBase** plots, 
				   const char** names)
{
  *plots++ = _hist;
  *plots++ = _hist_x;
  *plots++ = _hist_y;
  *plots++ = _chartx;
  *plots++ = _charty;
  for (unsigned i=0; i<Nplots; i++) names[i] = Names[i];
  return Nplots;
}

const MonQtBase* MonConsumerImage::selected() const
{
  MonQtBase* v;
  switch(_selected) {
  case MonCanvas::Normal     : v = _hist  ; break;
  case MonCanvas::ProjectionX: v = _hist_x; break;
  case MonCanvas::ProjectionY: v = _hist_y; break;
  case MonCanvas::ChartX     : v = _chartx; break;
  case MonCanvas::ChartY     : v = _charty; break;
  default                    : v = 0; break;
  }
  return v;
}

void MonConsumerImage::select(Select selection)
{
  //
  //  Disable the old plot
  //  
  MonQtImageDisplay* NoDisp(NULL);
  QwtPlot*           NoPlot(NULL);
  switch(_selected) {
  case MonCanvas::Normal     : _hist   ->attach(NoDisp); break;
  case MonCanvas::ProjectionX: _hist_x ->attach(NoPlot); break;
  case MonCanvas::ProjectionY: _hist_y ->attach(NoPlot); break;
  case MonCanvas::ChartX     : _chartx ->attach(NoPlot); 
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw); break;
  case MonCanvas::ChartY     : _charty ->attach(NoPlot);
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw); break;
  default: break;
  }

  _selected = selection;

  //
  //  Enable the new plot
  //
  switch(selection) {
  case MonCanvas::Normal     : _hist   ->attach(_frame); break;
  case MonCanvas::ProjectionX: _hist_x ->attach(_plot); break;
  case MonCanvas::ProjectionY: _hist_y ->attach(_plot); break;
  case MonCanvas::ChartX     : _chartx ->attach(_plot); break;
  case MonCanvas::ChartY     : _charty ->attach(_plot); break;
  default: break;
  }

  if (selection <= MonCanvas::Difference) {
    _stack->setCurrentIndex(Image); 
    _frame->show(); 
    return;
  }

  QFont titleFont(QApplication::font());
  titleFont.setPointSize(9);
  QwtText xtitle,ytitle;

  if (_selected == MonCanvas::ChartX ||
      _selected == MonCanvas::ChartY) {
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new MonTimeScale);
    _plot->setAxisLabelRotation (QwtPlot::xBottom, -50.0);
    _plot->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignLeft | Qt::AlignBottom);
    //     QwtScaleWidget *scaleWidget = _plot->axisWidget(QwtPlot::xBottom);
    //     const int fmh = QFontMetrics(scaleWidget->font()).height();
    //     scaleWidget->setMinBorderDist(0, fmh / 2);
    xtitle.setText("Time");
    ytitle.setText(_selected==MonCanvas::ChartX ? _entry->desc().xtitle() : _entry->desc().ytitle());
  }
  else {
    xtitle.setText(_entry->desc().xtitle());
    ytitle.setText(_entry->desc().ytitle());
  }

  _plot->setAxisTitle(QwtPlot::xBottom,xtitle);
  _plot->setAxisTitle(QwtPlot::yLeft  ,ytitle);

  _stack->setCurrentIndex(Plots);
}

