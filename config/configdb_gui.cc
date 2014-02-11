#include "pdsapp/config/Ui.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Parameters.hh"  // set edit mode

#include <QtGui/QApplication>

#include <stdio.h>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  bool edit=false;
  bool dbnamed=false;
  string path;
  bool lusage=false;

  for(int i=1; i<argc; i++) {
    if (strcmp(argv[i],"--edit")==0) edit=true;
    else if (strcmp(argv[i],"--db")==0) {
      path = string(argv[++i]);
      dbnamed = true;
    }
    else lusage=true;
  }
  lusage |= !dbnamed;
  if (lusage) {
    printf("%s --sql <path> [--edit]\n",argv[0]);
    return 1;
  }

  QApplication app(argc, argv);

  Experiment db(path.c_str(),edit?Experiment::Lock:Experiment::NoLock);

  Ui* ui = new Ui(db,edit);
  ui->show();
  app.exec();

  //  printf("\n==AFTER==\n");
  //  db.dump();
}
