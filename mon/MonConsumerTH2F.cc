#include <string.h>

#include "MonConsumerTH2F.hh"
#include "MonQtTH1F.hh"
#include "MonQtTH2F.hh"
#include "MonQtChart.hh"
#include "MonDialog.hh"
#include "MonTimeScale.hh"

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH2F.hh"
#include "pds/mon/MonDescTH2F.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include "qwt_plot.h"

using namespace Pds;

MonConsumerTH2F::MonConsumerTH2F(QWidget& parent,
				 const MonDesc& clientdesc,
				 const MonGroup& group, 
				 const MonEntryTH2F& entry) :
  MonCanvas(parent, entry),
  _group(group),
  _last(new MonEntryTH2F(entry.desc())),
  _hist(0),
  _diff(0),
  _hist_x(0),
  _hist_y(0),
  _diff_x(0),
  _diff_y(0),
  _chartx(0),
  _charty(0),
  _dialog(0)
{
  // Prepares menus
  _menu_service(Integrated, false);
  //  _menu_service(Since     , false);
  _menu_service(Difference, true);
  _menu_service(IntegratedX, false);
  _menu_service(IntegratedY, false);
  _menu_service(DifferenceX, false);
  _menu_service(DifferenceY, false);
  _menu_service(ChartX    , false);
  _menu_service(ChartY    , false);

  // Initialize histograms
  const MonDescTH2F& desc = _last->desc();
  const char* clientname = clientdesc.name();
  const char* dirname = group.desc().name();
  const char* entryname = desc.name();

  char tmp[128];
  snprintf(tmp, 128, "%s:%s:%s:HIST", clientname, dirname, entryname);
  _hist = new MonQtTH2F(tmp, desc);

  snprintf(tmp,  128, "%s:%s:%s:DIFF", clientname, dirname, entryname);
  _diff = new MonQtTH2F(tmp, desc);

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

  _plot = new QwtPlot(this);
  connect( this, SIGNAL(redraw()), _plot, SLOT(replot()) );
  layout()->addWidget(_plot);
  _plot->setAutoDelete(false);

  select(Difference);
}

MonConsumerTH2F::~MonConsumerTH2F() 
{
  delete _last;
  delete _hist;
  delete _diff;
  delete _hist_x;
  delete _hist_y;
  delete _diff_x;
  delete _diff_y;
  delete _chartx;
  delete _charty;
  if (_dialog) delete _dialog;
}

void MonConsumerTH2F::dialog()
{
  if (!_dialog)
    _dialog = new MonDialog(this, _hist, _diff, _hist_x, _hist_y,
                            _diff_x, _diff_y, _chartx, _charty);
  _dialog->show();
}

int MonConsumerTH2F::_update() 
{
  const MonEntryTH2F* entry = dynamic_cast<const MonEntryTH2F*>(_entry);
  if (entry->time() > _last->time()) {
    _diff->setto(*entry, *_last);
    _diff->stats();
    _hist->setto(*entry);
    _hist->stats();
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

int MonConsumerTH2F::_reset()
{
  _entry = _group.entry(_entry->desc().name());
  if (_entry && _entry->desc().type() == MonDescEntry::TH2F) {
    const MonEntryTH2F* entry = dynamic_cast<const MonEntryTH2F*>(_entry);
    const MonDescTH2F& desc = entry->desc();
    _last->params(desc);
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

static const unsigned Nplots = 8;
static const char* Names[Nplots] = {
  "Integrated",
  "Difference",
  "IntegratedX",
  "IntegratedY",
  "DifferenceX",
  "DifferenceY",
  "StripChartX",
  "StripChartY"
};

unsigned MonConsumerTH2F::getplots(MonQtBase** plots, 
				   const char** names)
{
  *plots++ = _hist;
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

const MonQtBase* MonConsumerTH2F::selected() const
{
  MonQtBase* v;
  switch(_selected) {
  case MonCanvas::Difference : v = _diff   ; break;
  case MonCanvas::DifferenceX: v = _diff_x ; break;
  case MonCanvas::DifferenceY: v = _diff_y ; break;
  case MonCanvas::Integrated : v = _hist   ; break;
  case MonCanvas::IntegratedX: v = _hist_x ; break;
  case MonCanvas::IntegratedY: v = _hist_y ; break;
  case MonCanvas::ChartX     : v = _chartx ; break;
  case MonCanvas::ChartY     : v = _charty ; break;
  default                    : v = 0       ; break;
  }
  return v;
}

void MonConsumerTH2F::select(Select selection)
{
  //   if (selection==Since) {
  //     _prev->setto(*_last);
  //   }
  
  if (!_plot) return;

  switch(_selected) {
  case MonCanvas::Difference : _diff   ->attach(NULL); break;
  case MonCanvas::DifferenceX: _diff_x ->attach(NULL); break;
  case MonCanvas::DifferenceY: _diff_y ->attach(NULL); break;
  case MonCanvas::Integrated : _hist   ->attach(NULL); break;
  case MonCanvas::IntegratedX: _hist_x ->attach(NULL); break;
  case MonCanvas::IntegratedY: _hist_y ->attach(NULL); break;
  case MonCanvas::ChartX     : _chartx ->attach(NULL); break;
  case MonCanvas::ChartY     : _charty ->attach(NULL); break;
    //   case MonCanvas::Since      : _since ->attach(NULL); break;
  default: break;
  }

  if (_selected == MonCanvas::ChartX ||
      _selected == MonCanvas::ChartY) {
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw);
    //     QwtScaleWidget *scaleWidget = _plot->axisWidget(QwtPlot::xBottom);
    //     scaleWidget->setMinBorderDist(0, 0);
  }

  _selected = selection;

  switch(selection) {
  case MonCanvas::Difference : _diff   ->attach(_plot); break;
  case MonCanvas::DifferenceX: _diff_x ->attach(_plot); break;
  case MonCanvas::DifferenceY: _diff_y ->attach(_plot); break;
  case MonCanvas::Integrated : _hist   ->attach(_plot); break;
  case MonCanvas::IntegratedX: _hist_x ->attach(_plot); break;
  case MonCanvas::IntegratedY: _hist_y ->attach(_plot); break;
  case MonCanvas::ChartX     : _chartx ->attach(_plot); break;
  case MonCanvas::ChartY     : _charty ->attach(_plot); break;
    //   case MonCanvas::Since      : _since ->attach(_plot); break;
  default: break;
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
}

void MonConsumerTH2F::join(MonCanvas& c)
{
  _plot = static_cast<MonConsumerTH2F&>(c)._plot;
}
