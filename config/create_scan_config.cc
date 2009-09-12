#include "pdsapp/config/ControlScan.hh"

#include <QtGui/QApplication>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  ControlScan* w = new ControlScan;
  w->show();

  app.exec();

  //  printf("\n==AFTER==\n");
  //  db.dump();
}
