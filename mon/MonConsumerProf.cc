#include <string.h>

#include "MonConsumerProf.hh"
#include "MonQtProf.hh"
#include "MonQtChart.hh"
#include "MonPath.hh"
#include "MonDialog.hh"
#include "MonTimeScale.hh"

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryProf.hh"
#include "pds/mon/MonDescProf.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include "qwt_plot.h"
#include <QtGui/QApplication>
#include <QtGui/QLayout>

using namespace Pds;

MonConsumerProf::MonConsumerProf(QWidget& parent,
				 const MonDesc& clientdesc,
				 const MonDesc& groupdesc, 
				 const MonEntryProf& entry) :
  MonCanvas(parent, entry),
  _last(new MonEntryProf(entry.desc())),
  _prev(new MonEntryProf(entry.desc())),
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
  const MonDescProf& desc = _last->desc();
  const char* clientname = clientdesc.name();
  const char* dirname = groupdesc.name();
  const char* entryname = desc.name();

  char tmp[128];
  snprintf(tmp, 128, "%s:%s:%s:PROF", clientname, dirname, entryname);
  _hist = new MonQtProf(tmp, desc);

  snprintf(tmp, 128, "%s:%s:%s:SINCE", clientname, dirname, entryname);
  _since = new MonQtProf(tmp, desc);

  snprintf(tmp,  128, "%s:%s:%s:DIFF", clientname, dirname, entryname);
  _diff = new MonQtProf(tmp, desc);

  snprintf(tmp, 128, "%s:%s:%s:CHART", clientname, dirname, entryname);
  _chart = new MonQtChart(tmp, desc);

  _plot = new QwtPlot(this);
  connect( this, SIGNAL(redraw()), _plot, SLOT(replot()) );
  layout()->addWidget(_plot);

  select(Difference);
}

MonConsumerProf::~MonConsumerProf() 
{
  delete _last;
  delete _prev;
  delete _hist;
  delete _since;
  delete _diff;
  delete _chart;
}

const MonQtBase* MonConsumerProf::selected() const
{
  MonQtBase* v;
  switch(_selected) {
  case MonCanvas::Difference: v = _diff ; break;
  case MonCanvas::Integrated: v = _hist ; break;
  case MonCanvas::Chart     : v = _chart; break;
  case MonCanvas::Since     : v = _since; break;
  default                   : v = 0; break;
  }
  return v;
}

void MonConsumerProf::select(Select selection)
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

  if (_selected == MonCanvas::Chart)
    _plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw);

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
    _plot->setAxisLabelRotation(QwtPlot::xBottom, -50.0);
    _plot->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignLeft | Qt::AlignBottom);
    xtitle.setText("Time");
    ytitle.setText(_entry->desc().ytitle());
  }
  else {
    xtitle.setText(_entry->desc().xtitle());
    ytitle.setText(_entry->desc().ytitle());
  }

  _plot->setAxisTitle(QwtPlot::xBottom,xtitle);
  _plot->setAxisTitle(QwtPlot::yLeft  ,ytitle);
}

void MonConsumerProf::dialog()
{
  new MonDialog(this, _hist, _since, _diff, _chart);
}

int MonConsumerProf::update() 
{
  const MonEntryProf* entry = dynamic_cast<const MonEntryProf*>(_entry);
  if (entry->time() > _last->time()) {
    _since->setto(*entry, *_prev);
    _diff ->setto(*entry, *_last);
    _hist ->setto(*entry);
    _chart->point(entry->last(), _diff->GetBinContents());
    _last ->setto(*entry);
    emit redraw();
    return 1;
  }
  return 0;
}

int MonConsumerProf::reset(const MonGroup& group)
{
  _entry = group.entry(_entry->desc().name());
  if (_entry && _entry->desc().type() == MonDescEntry::Prof) {
    const MonEntryProf* entry = dynamic_cast<const MonEntryProf*>(_entry);
    const MonDescProf& desc = entry->desc();
    _last ->params(desc);
    _prev ->params(desc);
    _chart->params(desc);
    return 1;
  }
  _entry = 0;
  return 0;
}

static const unsigned Nplots = 4;
static const char* Names[Nplots] = {"Integrated", "Since", "Difference", "Chart"};

unsigned MonConsumerProf::getplots(MonQtBase** plots, 
				       const char** names)
{
  *plots++ = _hist;
  *plots++ = _since;
  *plots++ = _diff;
  *plots++ = _chart;
  for (unsigned i=0; i<Nplots; i++) names[i] = Names[i];
  return Nplots;
}

