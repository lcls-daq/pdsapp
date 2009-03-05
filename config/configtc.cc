#include "ConfigTC_Gui.hh"
#include "ConfigTC_Opal1kConfig.hh"
#include "ConfigTC_FrameFexConfig.hh"
#include "ConfigTC_EvrConfig.hh"

#include <QtGui/QApplication>

#include <string.h>

using namespace ConfigGui;

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  ConfigTC_Gui* m = new ConfigTC_Gui;
  m->addSerializer( new EvrConfig );
  m->addSerializer( new Opal1kConfig );
  m->addSerializer( new FrameFexConfig );
  m->show();

  app.exec();
}
