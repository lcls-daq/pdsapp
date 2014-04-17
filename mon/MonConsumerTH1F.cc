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
#include <QtGui/QMessageBox>
#include <QtGui/QLayout>
#include <QtGui/QPixmap>
#include <QtCore/QTime>
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_widget.h"

using namespace Pds;

static QImage _qimage;

MonConsumerTH1F::MonConsumerTH1F(QWidget& parent,
				 const MonDesc& clientdesc,
				 const MonGroup& group, 
				 const MonEntryTH1F& entry) :
  MonCanvas(parent, entry),
  _group(group),
  _last(new MonEntryTH1F(entry.desc())),
  _prev(new MonEntryTH1F(entry.desc())),
  _hist(0),
  _since(0),
  _diff(0),
  _chart(0),
  _last_stats(0),
  _dialog(0),
  _archive_mode(false)
{
  // Prepares menus
  _menu_service(Integrated, false);
  _menu_service(Since     , false);
  _menu_service(Difference, true);
  _menu_service(Chart     , false);

  // Initialize histograms
  const MonDescTH1F& desc = _last->desc();
  const char* clientname = clientdesc.name();
  const char* dirname = group.desc().name();
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

  _last_stats = new MonStats1D;

  _plot = new QwtPlot(this);
  connect( this, SIGNAL(redraw()), _plot, SLOT(replot()) );
  layout()->addWidget(_plot);
  _plot->setAutoDelete(false);

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
  if (_dialog) delete _dialog;
  delete _last_stats;
}

void MonConsumerTH1F::dialog()
{
  if (!_dialog)
    _dialog = new MonDialog(this, _hist, _since, _diff, _chart);

  _dialog->show();
}

int MonConsumerTH1F::update() 
{
  const MonEntryTH1F* entry = dynamic_cast<const MonEntryTH1F*>(_entry);
  if (entry->time() > _last->time()) {
    if (_archive_mode) {
      MonStats1D diff;
      diff.setto(*entry, *_last_stats);
      if (diff.sum())
	_chart->point(entry->last(), diff.mean());
      _last_stats->setto(*entry);
    }
    else {
      _since->setto(*entry, *_prev);
      _since->stats();
      _diff ->setto(*entry, *_last);
      _diff ->stats();
      _hist ->setto(*entry);
      _hist ->stats();
      _chart->point(entry->last(), _diff->mean());
      _last ->setto(*entry);
      emit redraw();
    }
    return 1;
  }
  return 0;
}

int MonConsumerTH1F::replot()
{
  emit redraw();
  return 1;
}

int MonConsumerTH1F::reset()
{
  _last_stats->reset();
  _entry = _group.entry(_entry->desc().name());
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

void MonConsumerTH1F::archive_mode (unsigned n)
{
  _archive_mode=true;
  _chart->points(n);
  setChart();
  _select_group->setEnabled(false);
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

const MonQtBase* MonConsumerTH1F::selected() const
{
  MonQtBase* v;
  switch(_selected) {
  case MonCanvas::Difference: v = _diff ; break;
  case MonCanvas::Integrated: v = _hist ; break;
  case MonCanvas::Chart     : v = _chart; break;
  case MonCanvas::Since     : v = _since; break;
  default                   : v = 0     ; break;
  }
  return v;
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

void MonConsumerTH1F::info()
{
  double uflow, oflow, norm;
  switch(_selected) {
  case MonCanvas::Integrated: 
    uflow=_last->info(MonEntryTH1F::Underflow);
    oflow=_last->info(MonEntryTH1F::Overflow);
    norm =_last->info(MonEntryTH1F::Normalization);
    break;
  case MonCanvas::Difference: 
    uflow=_last->info(MonEntryTH1F::Underflow)    -_prev->info(MonEntryTH1F::Underflow);
    oflow=_last->info(MonEntryTH1F::Overflow)     -_prev->info(MonEntryTH1F::Overflow);
    norm =_last->info(MonEntryTH1F::Normalization)-_prev->info(MonEntryTH1F::Normalization);
    break;
  default: 
    return;
  }
  char msg_buffer[128];
  sprintf(msg_buffer,"Underflow %g\nOverflow %g\nNormalization %g",
	  uflow, oflow, norm);
  QMessageBox::information(this,"TH1F Info",msg_buffer);
}

void MonConsumerTH1F::join(MonCanvas& c)
{
  _plot = static_cast<MonConsumerTH1F&>(c)._plot;
  setVisible(false);
}

void MonConsumerTH1F::set_plot_color(unsigned icolor)
{
  _chart->color(icolor);
}
