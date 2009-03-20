#include "pdsapp/config/Ui.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/PdsDefs.hh"

#include <QtGui/QApplication>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  PdsDefs::initialize();

  string dbname(argv[1]);
  Experiment db(dbname);

  Ui* ui = new Ui(db);
  ui->show();
  app.exec();

  printf("\n==AFTER==\n");
  db.dump();
}
