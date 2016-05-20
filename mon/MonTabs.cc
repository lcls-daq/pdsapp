#include "MonTabs.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pdsapp/mon/MonCanvas.hh"
#include "pdsapp/mon/MonConsumerFactory.hh"

#include <QtGui/QScrollArea>
#include <QtGui/QGridLayout>

#include <errno.h>

static const unsigned Width = 900;
static const unsigned Height = 900;
static const unsigned Step = 8;
static const unsigned Empty = 0xffffffff;

using namespace Pds;

MyTab::MyTab( const MonGroup& group,
              unsigned        icolor) : QWidget(0)
{
  QGridLayout* layout = new QGridLayout;
  unsigned n = group.nentries();
  unsigned rows = (n+2)/3;
  unsigned columns = (n+rows-1)/rows;

  for(unsigned i = 0; i < group.nentries(); i++) {
    const MonEntry& entry = *group.entry(i);
    MonCanvas* canvas = MonConsumerFactory::create(*this,
						   "title",
						   group,
						   entry);
    canvas->set_plot_color(icolor);
    _canvases.push_back(canvas);
    layout->addWidget(canvas, i/columns, i%columns);
  }
  setLayout(layout);
}

void MyTab::insert(const MonGroup& group,
                   unsigned        icolor)
{
  for(unsigned i = 0; i < group.nentries(); i++) {
    const MonEntry& entry = *group.entry(i);
    MonCanvas* canvas = MonConsumerFactory::create(*this,
						   "title",
						   group,
						   entry);
    canvas->set_plot_color(icolor);
    _canvases[i]->overlay(*canvas);
  }
}

void MyTab::update(bool lredraw)
{
  for(std::vector<MonCanvas*>::iterator it=_canvases.begin(); it!=_canvases.end(); it++) {
    (*it)->update();
    if (lredraw)
      emit (*it)->replot();
  }
}

void MyTab::reset(unsigned n)
{
  for(std::vector<MonCanvas*>::iterator it=_canvases.begin(); it!=_canvases.end(); it++) {
    (*it)->reset();
    (*it)->archive_mode(n);
  }
}

MonTabs::MonTabs(QWidget& parent) : 
  QTabWidget(&parent)
{
}

MonTabs::~MonTabs() 
{
}

void MonTabs::reset(unsigned n)
{
  for(int i=0; i<count(); i++) {
    QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
    static_cast<MyTab*>(scroll->widget())->reset(n);
  }
}

void MonTabs::clear()
{
  while(count()) {
    QWidget* w = widget(0);
    removeTab(0);
    delete w;
  }
}

void MonTabs::setup(const MonCds& cds, unsigned icolor)
{
  for(unsigned tabid=0; tabid<cds.ngroups(); tabid++) {
    const MonGroup& group = *cds.group(tabid);
    bool lHasTab=false;
    QString gname(group.desc().name());
    for(int i=0; i<count(); i++)
      if (tabText(i)==gname) {
        lHasTab=true;
        QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
        static_cast<MyTab*>(scroll->widget())->insert(group,icolor);
        break;
      }

    if (!lHasTab) {
      MyTab* tab = new MyTab(group,icolor);
      QScrollArea* area = new QScrollArea(0);
      area->setWidget(tab);
      area->setWidgetResizable(true);
      addTab(area,group.desc().name());
    }
  }
}

void MonTabs::update(bool lredraw)
{
  for(int i=0; i<count(); i++) {
    QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
    static_cast<MyTab*>(scroll->widget())->update(lredraw);
  }
}  

void MonTabs::update(const MonCds& cds)
{
  for(unsigned tabid=0; tabid<cds.ngroups(); tabid++) {
    const MonGroup& group = *cds.group(tabid);
    QString gname(group.desc().name());
    for(int i=0; i<count(); i++) {
      if (tabText(i)==gname) {
        QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
        static_cast<MyTab*>(scroll->widget())->update(true);
        break;
      }
    }
  }
}  
