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
				 const MonDesc& groupdesc, 
				 const MonEntryImage& entry) :
  MonCanvas(parent, entry),
  _last(new MonEntryImage(entry.desc())),
  _prev(new MonEntryImage(entry.desc())),
  _hist(0),
  _since(0),
  _diff(0),
  _hist_x(0),
  _hist_y(0),
  _diff_x(0),
  _diff_y(0),
  _chartx(0),
  _charty(0)
{
  // Prepares menus
  _menu_service(Integrated, false);
  _menu_service(Since     , false);
  _menu_service(Difference, true);
  _menu_service(IntegratedX, false);
  _menu_service(IntegratedY, false);
  _menu_service(DifferenceX, false);
  _menu_service(DifferenceY, false);
  _menu_service(ChartX    , false);
  _menu_service(ChartY    , false);

  // Initialize histograms
  const MonDescImage& desc = _last->desc();
  const char* clientname = clientdesc.name();
  const char* dirname = groupdesc.name();
  const char* entryname = desc.name();

  char tmp[128];
  snprintf(tmp, 128, "%s:%s:%s:HIST", clientname, dirname, entryname);
  _hist = new MonQtImage(tmp, desc);

  snprintf(tmp, 128, "%s:%s:%s:SINCE", clientname, dirname, entryname);
  _since = new MonQtImage(tmp, desc);

  snprintf(tmp,  128, "%s:%s:%s:DIFF", clientname, dirname, entryname);
  _diff = new MonQtImage(tmp, desc);

  snprintf(tmp,  128, "%s:%s:%s:HISTX", clientname, dirname, entryname);
  _hist_x = new MonQtTH1F(tmp, desc, MonQtBase::X);

  snprintf(tmp,  128, "%s:%s:%s:HISTY", clientname, dirname, entryname);
  _hist_y = new MonQtTH1F(tmp, desc, MonQtBase::Y);

  snprintf(tmp,  128, "%s:%s:%s:DIFFX", clientname, dirname, entryname);
  _diff_x = new MonQtTH1F(tmp, desc, MonQtBase::X);

  snprintf(tmp,  128, "%s:%s:%s:DIFFY", clientname, dirname, entryname);
  _diff_y = new MonQtTH1F(tmp, desc, MonQtBase::Y);

  snprintf(tmp, 128, "%s:%s:%s:CHARTX", clientname, dirname, entryname);
  _chartx = new MonQtChart(tmp, desc, MonQtBase::X);

  snprintf(tmp, 128, "%s:%s:%s:CHARTY", clientname, dirname, entryname);
  _charty = new MonQtChart(tmp, desc, MonQtBase::Y);

  _frame = new MonQtImageDisplay(NULL);
  _plot  = new QwtPlot;
  connect( this, SIGNAL(redraw()), _frame , SLOT(display()) );
  connect( this, SIGNAL(redraw()), _plot  , SLOT(replot()) );

  _stack = new QStackedWidget(this);
  _stack->addWidget(_frame);
  _stack->addWidget(_plot);

  layout()->addWidget(_stack);

  select(Difference);
}

MonConsumerImage::~MonConsumerImage() 
{
  delete _last;
  delete _prev;
  delete _hist;
  delete _since;
  delete _diff;
  delete _hist_x;
  delete _hist_y;
  delete _diff_x;
  delete _diff_y;
  delete _chartx;
  delete _charty;
}

void MonConsumerImage::dialog()
{
  new MonDialog(this,
		_hist, _since, _diff, 
		_hist_x, _hist_y,
		_diff_x, _diff_y,
		_chartx, _charty);
}

int MonConsumerImage::update() 
{
  const MonEntryImage* entry = dynamic_cast<const MonEntryImage*>(_entry);
  if (entry->time() > _last->time()) {
    _since->setto(*entry, *_prev);
    _diff ->setto(*entry, *_last);
    _hist->setto(*entry);
    _hist->projectx(_hist_x);
    _hist->projecty(_hist_y);
    _diff->projectx(_diff_x);
    _diff->projecty(_diff_y);
    _hist_x->stats();
    _hist_y->stats();
    _diff_x->stats();
    _diff_y->stats();
    _chartx->point(entry->last(), _diff_x->mean());
    _charty->point(entry->last(), _diff_y->mean());
    _last->setto(*entry);
    emit redraw();
    return 1;
  }
  return 0;
}

int MonConsumerImage::reset(const MonGroup& group)
{
  _entry = group.entry(_entry->desc().name());
  if (_entry && _entry->desc().type() == MonDescEntry::Image) {
    const MonEntryImage* entry = dynamic_cast<const MonEntryImage*>(_entry);
    const MonDescImage& desc = entry->desc();
    _last->params(desc);
    _prev->params(desc);
    _hist->params(desc);
    _diff->params(desc);
    _hist_x->params(desc);
    _hist_y->params(desc);
    _diff_x->params(desc);
    _diff_y->params(desc);
    _chartx->params(desc);
    _charty->params(desc);
    return 1;
  }
  _entry = 0;
  return 0;
}

static const unsigned Nplots = 9;
static const char* Names[Nplots] = {
  "Integrated",
  "Since",
  "Difference",
  "IntegratedX",
  "IntegratedY",
  "DifferenceX",
  "DifferenceY",
  "StripChartX",
  "StripChartY"
};

unsigned MonConsumerImage::getplots(MonQtBase** plots, 
				   const char** names)
{
  *plots++ = _hist;
  *plots++ = _since;
  *plots++ = _diff;
  *plots++ = _hist_x;
  *plots++ = _hist_y;
  *plots++ = _diff_x;
  *plots++ = _diff_y;
  *plots++ = _chartx;
  *plots++ = _charty;
  for (unsigned i=0; i<Nplots; i++) names[i] = Names[i];
  return Nplots;
}

void MonConsumerImage::select(Select selection)
{
  if (selection==Since) {
    _prev->setto(*_last);
  }

  //
  //  Disable the old plot
  //  
  MonQtImageDisplay* NoDisp(NULL);
  QwtPlot*           NoPlot(NULL);
  switch(_selected) {
  case MonCanvas::Integrated : _hist   ->attach(NoDisp); break;
  case MonCanvas::Since      : _since  ->attach(NoDisp); break;
  case MonCanvas::Difference : _diff   ->attach(NoDisp); break;
  case MonCanvas::DifferenceX: _diff_x ->attach(NoPlot); break;
  case MonCanvas::DifferenceY: _diff_y ->attach(NoPlot); break;
  case MonCanvas::IntegratedX: _hist_x ->attach(NoPlot); break;
  case MonCanvas::IntegratedY: _hist_y ->attach(NoPlot); break;
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
  case MonCanvas::Integrated : _hist   ->attach(_frame); break;
  case MonCanvas::Since      : _since  ->attach(_frame); break;
  case MonCanvas::Difference : _diff   ->attach(_frame); break;
  case MonCanvas::DifferenceX: _diff_x ->attach(_plot); break;
  case MonCanvas::DifferenceY: _diff_y ->attach(_plot); break;
  case MonCanvas::IntegratedX: _hist_x ->attach(_plot); break;
  case MonCanvas::IntegratedY: _hist_y ->attach(_plot); break;
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

