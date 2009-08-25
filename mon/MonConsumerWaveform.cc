#include <string.h>

#include "MonConsumerWaveform.hh"
#include "MonQtWaveform.hh"
#include "MonQtChart.hh"
#include "MonDialog.hh"
#include "MonTimeScale.hh"

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryWaveform.hh"
#include "pds/mon/MonDescWaveform.hh"

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

MonConsumerWaveform::MonConsumerWaveform(QWidget& parent,
					 const MonDesc& clientdesc,
					 const MonDesc& groupdesc, 
					 const MonEntryWaveform& entry) :
  MonCanvas(parent, entry),
  _desc(new MonDescWaveform(entry.desc())),
  _hist(0),
  _time(0,0)
{
  // Initialize histograms
  const MonDescWaveform& desc = *_desc;
  const char* clientname = clientdesc.name();
  const char* dirname = groupdesc.name();
  const char* entryname = desc.name();

  char tmp[128];
  snprintf(tmp, 128, "%s:%s:%s:HIST", clientname, dirname, entryname);
  _hist = new MonQtWaveform(tmp,desc);
  
  _plot = new QwtPlot(this);
  connect( this, SIGNAL(redraw()), _plot, SLOT(replot()) );
  layout()->addWidget(_plot);

  // Prepares menus
  _menu_service(Normal, true);
  select(Normal);
}

MonConsumerWaveform::~MonConsumerWaveform() 
{
  delete _desc;
  delete _hist;
}

void MonConsumerWaveform::dialog()
{
  new MonDialog(this, _hist);
}

int MonConsumerWaveform::update() 
{
  const MonEntryWaveform* entry = dynamic_cast<const MonEntryWaveform*>(_entry);
  if (entry->time() > _time) {
    _hist ->setto(*entry);
    _time = entry->time();
    emit redraw();
    return 1;
  }
  return 0;
}

int MonConsumerWaveform::reset(const MonGroup& group)
{
  _entry = group.entry(_entry->desc().name());
  if (_entry && _entry->desc().type() == MonDescEntry::Waveform) {
    const MonEntryWaveform* entry = dynamic_cast<const MonEntryWaveform*>(_entry);
    *_desc = entry->desc();
    _hist ->params(*_desc);
    return 1;
  }
  _entry = 0;
  return 0;
}

static const unsigned Nplots = 1;
static const char* Names[Nplots] = {"Normal"};

unsigned MonConsumerWaveform::getplots(MonQtBase** plots, 
				       const char** names)
{
  *plots++ = _hist;
  for (unsigned i=0; i<Nplots; i++) names[i] = Names[i];
  return Nplots;
}

const MonQtBase* MonConsumerWaveform::selected() const
{
  return _hist;
}

void MonConsumerWaveform::select(Select selection)
{
  printf("select %d plot %p hist %p\n", selection, _plot, _hist);

  if (!_plot) return;

  _selected = selection;

  _hist ->attach(_plot);

  QFont titleFont(QApplication::font());
  titleFont.setPointSize(9);
  QwtText xtitle,ytitle;

  xtitle.setText(_entry->desc().xtitle());
  ytitle.setText(_entry->desc().ytitle());

  _plot->setAxisTitle(QwtPlot::xBottom,xtitle);
  _plot->setAxisTitle(QwtPlot::yLeft  ,ytitle);
}

void MonConsumerWaveform::info()
{
  /*
  double uflow, oflow, norm;
  uflow=_hist->info(MonEntryWaveform::Underflow);
  oflow=_hist->info(MonEntryWaveform::Overflow);
  norm =_hist->info(MonEntryWaveform::Normalization);

  char msg_buffer[128];
  sprintf(msg_buffer,"Underflow %g\nOverflow %g\nNormalization %g",
	  uflow, oflow, norm);
  QMessageBox::information(this,"Waveform Info",msg_buffer);
  */
}

