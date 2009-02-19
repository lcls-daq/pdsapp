#include <string.h>

#include "MonConsumerTH1F.hh"
#include "MonQtTH1F.hh"
#include "MonQtChart.hh"
#include "MonDialog.hh"
#include "MonTimeScale.hh"

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonDescTH1F.hh"

#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QLayout>
#include <QtCore/QTime>
#include "qwt_plot.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_widget.h"

using namespace Pds;

MonConsumerTH1F::MonConsumerTH1F(QWidget& parent,
				 const MonDesc& clientdesc,
				 const MonDesc& groupdesc, 
				 const MonEntryTH1F& entry) :
  MonCanvas(parent, entry),
  _last(new MonEntryTH1F(entry.desc())),
  _prev(new MonEntryTH1F(entry.desc())),
  _hist(0),
  _since(0),
  _diff(0),
  _chart(0)
{
  // Prepares menus
  _menu_service(Integrated, false);
  _menu_service(Since     , false);
  _menu_service(Difference, true);
  _menu_service(Chart     , false);

  // Initialize histograms
  const MonDescTH1F& desc = _last->desc();
  const char* clientname = clientdesc.name();
  const char* dirname = groupdesc.name();
  const char* entryname = desc.name();

  char tmp[128];
  snprintf(tmp, 128, "%s:%s:%s:HIST", clientname, dirname, entryname);
  _hist = new MonQtTH1F(tmp,desc);
  
  snprintf(tmp, 128, "%s:%s:%s:SINCE", clientname, dirname, entryname);
  _since = new MonQtTH1F(tmp,desc);
  
  snprintf(tmp,  128, "%s:%s:%s:DIFF", clientname, dirname, entryname);
  _diff = new MonQtTH1F(tmp,desc);
  
  snprintf(tmp, 128, "%s:%s:%s:CHART", clientname, dirname, entryname);
  _chart = new MonQtChart(tmp,desc);

  _plot = new QwtPlot(this);
  connect( this, SIGNAL(redraw()), _plot, SLOT(replot()) );
  layout()->addWidget(_plot);

  select(Difference);
}

MonConsumerTH1F::~MonConsumerTH1F() 
{
  delete _last;
  delete _prev;
  delete _hist;
  delete _since;
  delete _diff;
  delete _chart;
}

void MonConsumerTH1F::dialog()
{
  new MonDialog(this, _hist, _since, _diff, _chart);
}

int MonConsumerTH1F::update() 
{
  const MonEntryTH1F* entry = dynamic_cast<const MonEntryTH1F*>(_entry);
  if (entry->time() > _last->time()) {
    _since->setto(*entry, *_prev);
    _since->stats();
    _diff ->setto(*entry, *_last);
    _diff ->stats();
    _hist ->setto(*entry);
    _hist ->stats();
    _chart->point(entry->last(), _diff->mean());
    _last ->setto(*entry);
    emit redraw();
    return 1;
  }
  return 0;
}

int MonConsumerTH1F::reset(const MonGroup& group)
{
  _entry = group.entry(_entry->desc().name());
  if (_entry && _entry->desc().type() == MonDescEntry::TH1F) {
    const MonEntryTH1F* entry = dynamic_cast<const MonEntryTH1F*>(_entry);
    _last ->params(entry->desc());
    _prev ->params(entry->desc());
    _hist ->params(entry->desc());
    _since->params(entry->desc());
    _diff ->params(entry->desc());
    _chart->params(entry->desc());
    return 1;
  }
  _entry = 0;
  return 0;
}

static const unsigned Nplots = 4;
static const char* Names[Nplots] = {"Integrated", "Since", "Difference", "Chart"};

unsigned MonConsumerTH1F::getplots(MonQtBase** plots, 
				   const char** names)
{
  *plots++ = _hist;
  *plots++ = _since;
  *plots++ = _diff;
  *plots++ = _chart;
  for (unsigned i=0; i<Nplots; i++) names[i] = Names[i];
  return Nplots;
}

void MonConsumerTH1F::select(Select selection)
{
  if (selection==Since) {
    _prev->setto(*_last);
  }
  
  if (!_plot) return;

  switch(_selected) {
  case MonCanvas::Difference: _diff ->attach(NULL); break;
  case MonCanvas::Integrated: _hist ->attach(NULL); break;
  case MonCanvas::Chart     : _chart->attach(NULL); break;
  case MonCanvas::Since     : _since->attach(NULL); break;
  default: break;
  }

  if (_selected == MonCanvas::Chart) {
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw);
//     QwtScaleWidget *scaleWidget = _plot->axisWidget(QwtPlot::xBottom);
//     scaleWidget->setMinBorderDist(0, 0);
  }

  _selected = selection;

  switch(selection) {
  case MonCanvas::Difference: _diff ->attach(_plot); break;
  case MonCanvas::Integrated: _hist ->attach(_plot); break;
  case MonCanvas::Chart     : _chart->attach(_plot); break;
  case MonCanvas::Since     : _since->attach(_plot); break;
  default: break;
  }

  QFont titleFont(QApplication::font());
  titleFont.setPointSize(9);
  QwtText xtitle,ytitle;

  if (_selected == MonCanvas::Chart) {
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new MonTimeScale);
    _plot->setAxisLabelRotation (QwtPlot::xBottom, -50.0);
    _plot->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignLeft | Qt::AlignBottom);
//     QwtScaleWidget *scaleWidget = _plot->axisWidget(QwtPlot::xBottom);
//     const int fmh = QFontMetrics(scaleWidget->font()).height();
//     scaleWidget->setMinBorderDist(0, fmh / 2);
    xtitle.setText("Time");
    ytitle.setText(_entry->desc().xtitle());
  }
  else {
    xtitle.setText(_entry->desc().xtitle());
    ytitle.setText(_entry->desc().ytitle());
  }

  _plot->setAxisTitle(QwtPlot::xBottom,xtitle);
  _plot->setAxisTitle(QwtPlot::yLeft  ,ytitle);
}
