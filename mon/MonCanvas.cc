#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include "MonCanvas.hh"
#include "MonQtBase.hh"

#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescEntry.hh"

#include "pdsdata/xtc/ClockTime.hh"

using namespace Pds;

MonCanvas::MonCanvas(QWidget&        parent, 
		     const MonEntry& entry) :
  QGroupBox(entry.desc().name(),&parent),
  _entry(&entry),
  _selected(Undefined)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QMenuBar* menu_bar = new QMenuBar(this);
  {
    QMenu* file_menu = new QMenu("File");
    file_menu->addAction("Close", this, SLOT(close()));
    file_menu->addSeparator();
    file_menu->addAction("Save image", this, SLOT(save_image()));
    file_menu->addAction("Save data" , this, SLOT(save_data ()));
    menu_bar->addMenu(file_menu);

    _select = new QMenu("Select");
    QAction* action = new QAction("Settings", _select);
    _select->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(settings()));
    _select->addSeparator();
    _select_group = new QActionGroup(_select);
    menu_bar->addMenu(_select);

    menu_bar->addAction("Info", this, SLOT(show_info()));
  }
  layout->addWidget(menu_bar);
  setLayout(layout);
}

MonCanvas::~MonCanvas() 
{
  delete _select;
}

void MonCanvas::menu_service(Select s, const char* label, const char* slot,
			     bool lInit) {
  QAction* action = new QAction( label, _select ); 
  action->setCheckable(true); 
  _select->addAction(action); 
  connect(action, SIGNAL(triggered()), this, slot );
  _select_group->addAction(action);
  action->setChecked(lInit);
}

void MonCanvas::close() {}

void MonCanvas::save_image()  
{
  char time_buffer[32];
  time_t seq_tm = _entry->time().seconds();
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def =_entry->desc().name();
  def += "_";
  def += time_buffer;
  def += ".bmp";
  QString fname = 
    QFileDialog::getSaveFileName(this,"Save File As (.bmp,.jpg,.png)",
				 def,".bmp;.png;.jpg");
  if (!fname.isNull()) {
    QPixmap pixmap(QWidget::size());
    QWidget::render(&pixmap);
    pixmap.toImage().save(fname);
  }
}

void MonCanvas::save_data()  
{
  char time_buffer[32];
  time_t seq_tm = _entry->time().seconds();
  strftime(time_buffer,32,"%Y%m%d_%H%M%S",localtime(&seq_tm));

  QString def =_entry->desc().name();
  def += "_";
  def += time_buffer;
  def += ".dat";
  QString fname = 
    QFileDialog::getSaveFileName(this,"Save File As (.dat)",
				 def,".dat");
  if (!fname.isNull()) {
    FILE* f = fopen(qPrintable(fname),"w");
    if (!f)
      QMessageBox::warning(this, "Save data", 
			   QString("Error opening %1 for writing").arg(fname));
    else {
      const MonQtBase* v = selected();
      if (v) 
	v->dump(f);
      else 
	QMessageBox::warning(this, "Save data", 
			     QString("Error grabbing data"));
      fclose(f);
    }
  }
}

void MonCanvas::info() {}
void MonCanvas::show_info() { info(); }

void MonCanvas::setNormal     () { select(Normal); }
void MonCanvas::setIntegrated () { select(Integrated); }
void MonCanvas::setSince      () { select(Since); }
void MonCanvas::setDifference () { select(Difference); }
void MonCanvas::setChart      () { select(Chart); }
void MonCanvas::setProjectionX() { select(ProjectionX); }
void MonCanvas::setProjectionY() { select(ProjectionY); }
void MonCanvas::setIntegratedX() { select(IntegratedX); }
void MonCanvas::setIntegratedY() { select(IntegratedY); }
void MonCanvas::setDifferenceX() { select(DifferenceX); }
void MonCanvas::setDifferenceY() { select(DifferenceY); }
void MonCanvas::setChartX     () { select(ChartX); }
void MonCanvas::setChartY     () { select(ChartY); }

void MonCanvas::select(Select selection)
{
  _selected = selection;
  for(unsigned i=0; i<_overlays.size(); i++)
    _overlays[i]->select(selection);
}

//const char* MonCanvas::name() const {return _entryname;}

static const unsigned MaxPlots = 8;
static const unsigned Naxis = 3;
static const char AxisNames[Naxis] = {'x', 'y', 'z'};

int MonCanvas::writeconfig(FILE* fp)
{
  MonQtBase* plots[MaxPlots];
  const char* names[MaxPlots];
  unsigned nplots = getplots(plots, names);

  fprintf(fp, "  Type %d Plots %d Show %d\n", 
	  _entry->desc().type(), nplots, _selected);

  for (unsigned i=0; i<nplots; i++) {
    for (unsigned a=0; a<Naxis; a++) {
      MonQtBase::Axis ax = MonQtBase::Axis(a);
      fprintf(fp, "  %s %c %f %f %d %d\n", 
	      names[i], AxisNames[a], 
	      plots[i]->min(ax), 
	      plots[i]->max(ax), 
	      plots[i]->isautorng(ax),
	      plots[i]->islog(ax));
    }
  }
  return 0;
}

int MonCanvas::readconfig(FILE* fp,int color)
{
  unsigned MaxLine=256;
  char line[MaxLine], eat[MaxLine], plotname[MaxLine];
  MonQtBase* plots[MaxPlots];
  const char* names[MaxPlots];
  unsigned nplots = getplots(plots, names);

  int itype;
  unsigned inplots;
  int isel;

  if (!fgets(line, MaxLine, fp)) return -1;
  sscanf(line, "%s %d %s %d %s %d", 
	 eat, &itype,
	 eat, &inplots, 
	 eat, &isel);
  if (inplots != nplots) return -1;

  Select sel = (Select)isel;

  for (unsigned i=0; i<nplots; i++) {
    for (unsigned a=0; a<Naxis; a++) {
      float min, max;
      int isautorng, islog;
      MonQtBase::Axis ax = MonQtBase::Axis(a);
      char axname;
      if (!fgets(line, MaxLine, fp)) return -1;
      sscanf(line, "%s %c %f %f %d %d", 
	     plotname, &axname, &min, &max, &isautorng, &islog);
      if (strcmp(plotname, names[i]) != 0 || axname != AxisNames[a]) return -1;
      plots[i]->settings(ax, min, max, isautorng, islog);
    }
    plots[i]->apply();
  }
  select(sel);
  redraw();
  return 0;
}

void MonCanvas::settings()
{
  dialog();
}

void MonCanvas::overlay(MonCanvas& c)
{
  _overlays.push_back(&c);
  c.join(*this);
  c.select(_selected);
}

int MonCanvas::update() {
  for(unsigned i=0; i<_overlays.size(); i++)
    _overlays[i]->_update();
  return _update();
}

int MonCanvas::replot() {
  for(unsigned i=0; i<_overlays.size(); i++)
    _overlays[i]->_replot();
  return _replot();
}

int MonCanvas::reset() {
  for(unsigned i=0; i<_overlays.size(); i++)
    _overlays[i]->_reset();
  return _reset();
}

void MonCanvas::archive_mode(unsigned n) {
  for(unsigned i=0; i<_overlays.size(); i++)
    _overlays[i]->_archive_mode(n);
  _archive_mode(n);
}

