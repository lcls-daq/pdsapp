#include <string.h>

#include "MonConsumerScalar.hh"
#include "MonQtChart.hh"
#include "MonDialog.hh"
#include "MonTimeScale.hh"

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryScalar.hh"
#include "pds/mon/MonDescScalar.hh"

#include "pdsdata/xtc/ClockTime.hh"

#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QPixmap>
#include <QtCore/QTime>
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_widget.h"

using namespace Pds;

static QImage _qimage;

MonConsumerScalar::MonConsumerScalar(QWidget& parent,
                                     const MonDesc& clientdesc,
                                     const MonGroup& group, 
                                     const MonEntryScalar& entry) :
  MonCanvas(parent, entry),
  _group(group),
  _last(new MonEntryScalar(entry.desc())),
  _prev(new MonEntryScalar(entry.desc())),
  _hist(0),
  _since(0),
  _diff(0),
  _dialog(0),
  _barchive_mode(false)
{
  // Prepares menus
  _menu_service(Integrated, false);
  _menu_service(Since     , false);
  _menu_service(Difference, true);

  // Initialize histograms
  const MonDescScalar& desc = entry.desc();
  const char* clientname = clientdesc.name();
  const char* dirname    = group.desc().name();
  const char* entryname  = entry.desc().name();

  char tmp[128];
  snprintf(tmp, 128, "%s:%s:%s:HIST", clientname, dirname, entryname);
  _hist = new MonQtChart(tmp,desc);

  snprintf(tmp, 128, "%s:%s:%s:SINCE", clientname, dirname, entryname);
  _since = new MonQtChart(tmp,desc);

  snprintf(tmp, 128, "%s:%s:%s:DIFF", clientname, dirname, entryname);
  _diff = new MonQtChart(tmp,desc);

  _plot = new QwtPlot(this);
  connect( this, SIGNAL(redraw()), _plot, SLOT(replot()) );
  layout()->addWidget(_plot);
  _plot->setAutoDelete(false);

  select(Difference);
}

MonConsumerScalar::~MonConsumerScalar() 
{
  delete _last;
  delete _prev;
  delete _hist;
  delete _since;
  delete _diff;
  if (_dialog) delete _dialog;
}

void MonConsumerScalar::dialog()
{
   if (!_dialog)
     _dialog = new MonDialog(this, _hist, _since, _diff);
  
   _dialog->show();
}

int MonConsumerScalar::_update() 
{
  const MonEntryScalar* entry = dynamic_cast<const MonEntryScalar*>(_entry);
  if (entry->time() > _last->time()) {

    _since->point(entry->last(), entry->since(*_prev)); 
    _diff ->point(entry->last(), entry->since(*_last));
    _hist ->point(entry->last(), entry->values());
    _last ->setto(*entry);

    if (!_barchive_mode)
      emit redraw();

    return 1;
  }
  return 0;
}

int MonConsumerScalar::_replot()
{
  emit redraw();
  return 1;
}

int MonConsumerScalar::_reset()
{
  _last->reset();
  _entry = _group.entry(_entry->desc().name());
  if (_entry && _entry->desc().type() == MonDescEntry::Scalar) {
    const MonEntryScalar* entry = dynamic_cast<const MonEntryScalar*>(_entry);
    _last ->params(entry->desc());
    _prev ->params(entry->desc());
    _hist ->params(entry->desc());
    _since->params(entry->desc());
    _diff ->params(entry->desc());
    return 1;
  }
  _entry = 0;
  return 0;
}

void MonConsumerScalar::_archive_mode (unsigned n)
{
  _barchive_mode=true;
   _hist ->points(n);
   _since->points(n);
   _diff ->points(n);
   _select_group->setEnabled(true);
   select(_selected);
}

static const unsigned Nplots = 3;
static const char* Names[Nplots] = {"Integrated", "Since", "Difference"};

unsigned MonConsumerScalar::getplots(MonQtBase** plots, 
				     const char** names)
{
  *plots++ = _hist;
  *plots++ = _since;
  *plots++ = _diff;
  for (unsigned i=0; i<Nplots; i++) names[i] = Names[i];
  return Nplots;
}

const MonQtBase* MonConsumerScalar::selected() const
{
  MonQtBase* v;
  switch(_selected) {
  case MonCanvas::Difference: v = _diff ; break;
  case MonCanvas::Integrated: v = _hist ; break;
  case MonCanvas::Since     : v = _since; break;
  default                   : v = 0     ; break;
  }
  return v;
}

void MonConsumerScalar::select(Select selection)
{
  if (selection==Since) {
    _prev->setto(*_last);
  }

  if (!_plot) return;

  switch(_selected) {
  case MonCanvas::Difference: _diff ->attach(NULL); break;
  case MonCanvas::Integrated: _hist ->attach(NULL); break;
  case MonCanvas::Since     : _since->attach(NULL); break;
  default: break;
  }

  _plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw);

  MonCanvas::select(selection);

  switch(selection) {
  case MonCanvas::Difference: _diff ->attach(_plot); break;
  case MonCanvas::Integrated: _hist ->attach(_plot); break;
  case MonCanvas::Since     : _since->attach(_plot); break;
  default: break;
  }

  QFont titleFont(QApplication::font());
  titleFont.setPointSize(9);
  QwtText xtitle,ytitle;

  _plot->setAxisScaleDraw(QwtPlot::xBottom, new MonTimeScale);
  _plot->setAxisLabelRotation (QwtPlot::xBottom, -50.0);
  _plot->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignLeft | Qt::AlignBottom);
  xtitle.setText("Time");
  if (_entry)
    ytitle.setText(_entry->desc().xtitle());

  _plot->setAxisTitle(QwtPlot::xBottom,xtitle);
  _plot->setAxisTitle(QwtPlot::yLeft  ,ytitle);

  emit redraw();
}

void MonConsumerScalar::info()
{
  QDialog* info = new QDialog(this);
  QGridLayout* l = new QGridLayout;
  for(unsigned i=0; i<_hist->nlines(); i++) {
    QColor  c = _hist->qcolor(i);
    QPalette p;
    p.setColor(QPalette::BrightText,c);
    p.setColor(QPalette::ButtonText,c);
    p.setColor(QPalette::WindowText,c);
    QLabel* q = new QLabel(_hist->name(i));
    q->setPalette(p);
    l->addWidget(q,i%16,i/16);
  }
  info->setLayout(l);
  info->show();
}

void MonConsumerScalar::join(MonCanvas& c)
{
  _plot = static_cast<MonConsumerScalar&>(c)._plot;
  setVisible(false);
}

void MonConsumerScalar::set_plot_color(unsigned icolor)
{
  _hist ->color(icolor);
  _since->color(icolor);
  _diff ->color(icolor);
}
