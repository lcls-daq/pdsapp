#include <errno.h>

#include "MonMain.hh"
//#include "MonLayoutHints.hh"
#include "MonTreeMenu.hh"
#include "MonTabMenu.hh"

#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtGui/QHBoxLayout>

using namespace Pds;

MonMain::MonMain(Task* workTask, 
		 const char** hosts, 
		 const char* config) : 
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
  _trees = new MonTreeMenu(*top, *_task, *_tabs, hosts, config);

  layout->addWidget(_trees,0);
  layout->addWidget(_tabs ,1);

  top->setLayout(layout);
  top->show();

  // Start our main ROOT timer
  start();

  app.exec();
}

MonMain::~MonMain() {}

void MonMain::expired() 
{
  _trees->expired();
}

