#include <QtGui/QMenuBar>
#include <QtGui/QActionGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>

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
  QPixmap pixmap(QWidget::size());
  QWidget::render(&pixmap);
  QImage img = pixmap.toImage();

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
  if (!fname.isNull())
    img.save(fname);
}

void MonCanvas::info() {}
void MonCanvas::show_info() { info(); }

void MonCanvas::setIntegrated () { select(Integrated); }
void MonCanvas::setSince      () { select(Since); }
void MonCanvas::setDifference () { select(Difference); }
void MonCanvas::setChart      () { select(Chart); }
void MonCanvas::setIntegratedX() { select(IntegratedX); }
void MonCanvas::setIntegratedY() { select(IntegratedY); }
void MonCanvas::setDifferenceX() { select(DifferenceX); }
void MonCanvas::setDifferenceY() { select(DifferenceY); }
void MonCanvas::setChartX     () { select(ChartX); }
void MonCanvas::setChartY     () { select(ChartY); }

void MonCanvas::select(Select selection)
{
  _selected = selection;
}

/*
void MonCanvas::saveas(const char* type, const char* groupname) 
{
  if (!strlen(type)) {
    const char* filetypes[] = {"Bitmap", "*.bmp",
			       "JPEG",   "*.jpg",
			       "PNG",    "*.png",
			       0,        0};
    TGFileInfo info;
    info.fFileTypes = filetypes;
    info.fFilename = _entryname;
    new TGFileDialog(fClient->GetQt(), GetParent(), kFDSave, &info);
    _canvas.GetCanvas()->SaveAs(info.fFilename);
  } else {
    unsigned len = strlen(groupname)+strlen(_entryname)+strlen(type)+3;
    char filename[len];
    sprintf(filename, "%s_%s.%s", groupname, _entryname, type);
    _canvas.GetCanvas()->SaveAs(filename);
  }
}
*/

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

  MonDescEntry::Type type;
  unsigned inplots;
  Select sel;

  if (!fgets(line, MaxLine, fp)) return -1;
  sscanf(line, "%s %d %s %d %s %d", 
	 eat, reinterpret_cast<int*>(&type), 
	 eat, &inplots, 
	 eat, reinterpret_cast<int*>(&sel));
  if (inplots != nplots) return -1;

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
