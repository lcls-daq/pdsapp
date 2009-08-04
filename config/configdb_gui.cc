#include "pdsapp/config/Ui.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Parameters.hh"  // set edit mode

#include <QtGui/QApplication>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  bool edit=false;
  bool dbnamed=false;
  string dbname;
  bool lusage=false;

  for(int i=1; i<argc; i++) {
    if (strcmp(argv[i],"--edit")==0) edit=true;
    else if (strcmp(argv[i],"--db")==0) {
      dbname = string(argv[++i]);
      dbnamed = true;
    }
    else lusage=true;
  }
  lusage |= !dbnamed;
  if (lusage) {
    printf("%s --db <dbname> [--edit]\n",argv[0]);
    return 1;
  }

  QApplication app(argc, argv);

  Experiment db(dbname);
  if (!db.is_valid()) {
    printf("Database root %s is not valid\n",dbname.data());
    if (!edit) return -1;

    printf("Create? [y/n]: ");
    const int maxlen=128;
    char line[maxlen];
    char* result = fgets(line, maxlen, stdin);
    if (*result!='y' && *result!='Y')
      return -1;

    db.create();
  }

  Parameter::allowEdit(edit);

  Ui* ui = new Ui(db,edit);
  ui->show();
  app.exec();

  //  printf("\n==AFTER==\n");
  //  db.dump();
}
