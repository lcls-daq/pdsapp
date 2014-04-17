#include "VmonReaderTabs.hh"

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
    std::vector<MonCanvas*> v;
    v.push_back(canvas);
    _canvases.push_back(v);
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
    _canvases[i].push_back(canvas);
    canvas->join(*(_canvases[i].front()));
  }
}

void MyTab::update(bool lredraw)
{
  for(std::vector<std::vector<MonCanvas*> >::iterator it=_canvases.begin(); it!=_canvases.end(); it++) {
    for(unsigned i=0; i<it->size(); i++) {
      (*it)[i]->update();
      if (lredraw)
        emit (*it)[i]->replot();
    }
  }
}

void MyTab::reset(unsigned n)
{
  for(std::vector<std::vector<MonCanvas*> >::iterator it=_canvases.begin(); it!=_canvases.end(); it++) {
    for(unsigned i=0; i<it->size(); i++) {
      (*it)[i]->reset();
      (*it)[i]->archive_mode(n);
    }
  }
}

VmonReaderTabs::VmonReaderTabs(QWidget& parent) : 
  QTabWidget(&parent)
{
}

VmonReaderTabs::~VmonReaderTabs() 
{
}

void VmonReaderTabs::reset(unsigned n)
{
  for(int i=0; i<count(); i++) {
    QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
    static_cast<MyTab*>(scroll->widget())->reset(n);
  }
}

void VmonReaderTabs::clear()
{
  while(count()) {
    QWidget* w = widget(0);
    removeTab(0);
    delete w;
  }
}

void VmonReaderTabs::setup(const MonCds& cds, unsigned icolor)
{
  printf("setup %s\n",cds.desc().name());
  for(unsigned tabid=0; tabid<cds.ngroups(); tabid++) {
    const MonGroup& group = *cds.group(tabid);
    printf("\t%s\n",group.desc().name());
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

void VmonReaderTabs::update(bool lredraw)
{
  for(int i=0; i<count(); i++) {
    QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
    static_cast<MyTab*>(scroll->widget())->update(lredraw);
  }
}  
