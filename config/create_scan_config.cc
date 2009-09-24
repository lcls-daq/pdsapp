#include "pdsapp/config/ControlScan.hh"

#include <QtGui/QApplication>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  QString fname;
  bool lusage=false;
  for(int i=1; i<argc; i++) {
    if (argv[i]=="-o") fname=QString(argv[++i]);
    else lusage=true;
  }

  if (lusage) {
    printf("%s -o <filename>\n",argv[0]);
    exit(1);
  }

  QApplication app(argc, argv);

  ControlScan* w = new ControlScan(fname);
  w->show();

  app.exec();

  //  printf("\n==AFTER==\n");
  //  db.dump();
}
