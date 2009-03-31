#include <errno.h>

#include "VmonMain.hh"
#include "VmonTreeMenu.hh"
#include "MonTabMenu.hh"

#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtGui/QHBoxLayout>

using namespace Pds;

VmonMain::VmonMain(Task* workTask, 
		   unsigned char platform,
		   const char* partition) :
  _task(workTask)
{
  int argc=1;
  char* argv[] = { "app", NULL };
  QApplication app(argc, argv);

  QWidget* top = new QWidget(0);
  QHBoxLayout* layout = new QHBoxLayout(top);

  // Tabs
  _tabs = new MonTabMenu(*top);

  // Tree(s)
  _trees = new VmonTreeMenu(*top, *_task, *_tabs, platform, partition);

  layout->addWidget(_trees,0);
  layout->addWidget(_tabs ,1);

  top->setLayout(layout);
  top->show();

  // Start our main ROOT timer
  start();

  app.exec();
}

VmonMain::~VmonMain() {}

void VmonMain::expired() 
{
  _trees->expired();
}

