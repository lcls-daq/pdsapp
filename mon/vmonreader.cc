#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "VmonReaderTreeMenu.hh"
#include "VmonReaderTabs.hh"

#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtGui/QHBoxLayout>

void printHelp(const char* program);

using namespace Pds;

int main(int argc, char **argv) 
{
  const char* path = ".";
  int c;
  while ((c = getopt(argc, argv, "p:")) != -1) {
    switch (c) {
    case 'p':
      path = optarg;
      break;
    default:
      printHelp(argv[0]);
      return 0;
    }
  }

  QApplication app(argc, argv);
  QWidget* top = new QWidget(0);
  QHBoxLayout* layout = new QHBoxLayout(top);

  // Tabs
  VmonReaderTabs*  _tabs = new VmonReaderTabs(*top);

  // Tree(s)
  VmonReaderTreeMenu* _trees = new VmonReaderTreeMenu(*top, *_tabs, path);

  layout->addWidget(_trees,0);
  layout->addWidget(_tabs ,1);

  top->setLayout(layout);
  top->show();

  app.exec();
}


void printHelp(const char* program)
{
  printf("usage: %s [-p <path>]\n", program);
}
