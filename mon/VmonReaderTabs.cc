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

MyTab::MyTab( const MonGroup& group ) : QWidget(0) , _group(group)
{
  QGridLayout* layout = new QGridLayout;
  unsigned n = group.nentries();
  unsigned rows = (n+2)/3;
  unsigned columns = (n+rows-1)/rows;

  for(unsigned i = 0; i < group.nentries(); i++) {
    const MonEntry& entry = *group.entry(i);
    MonCanvas* canvas = MonConsumerFactory::create(*this,
						   "title",
						   group.desc(),
						   entry);
    canvas->archive_mode();
    _canvases.push_back(canvas);
    layout->addWidget(canvas, i/columns, i%columns);
  }
  setLayout(layout);
}

void MyTab::update(bool lredraw)
{
  for(std::vector<MonCanvas*>::iterator it=_canvases.begin(); it!=_canvases.end(); it++) {
    (*it)->update();
    if (lredraw)
      emit (*it)->replot();
  }
}

void MyTab::reset()
{
  for(std::vector<MonCanvas*>::iterator it=_canvases.begin(); it!=_canvases.end(); it++)
    (*it)->reset(_group);
}

VmonReaderTabs::VmonReaderTabs(QWidget& parent) : 
  QTabWidget(&parent)
{
}

VmonReaderTabs::~VmonReaderTabs() 
{
}

void VmonReaderTabs::reset()
{
  for(int i=0; i<count(); i++) {
    QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
    static_cast<MyTab*>(scroll->widget())->reset();
  }
}

void VmonReaderTabs::reset(const MonCds& cds)
{
  while(count()) {
    QWidget* w = widget(0);
    removeTab(0);
    delete w;
  }

  for(unsigned tabid=0; tabid<cds.ngroups(); tabid++) {
    const MonGroup& group = *cds.group(tabid);
    MyTab* tab = new MyTab(group);
    QScrollArea* area = new QScrollArea(0);
    area->setWidget(tab);
    area->setWidgetResizable(true);
    addTab(area,group.desc().name());
  }
}

void VmonReaderTabs::update(bool lredraw)
{
  for(int i=0; i<count(); i++) {
    QScrollArea* scroll = static_cast<QScrollArea*>(widget(i));
    static_cast<MyTab*>(scroll->widget())->update(lredraw);
  }
}  
