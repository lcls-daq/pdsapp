#include "pdsapp/config/ListUi.hh"
#include "pdsapp/config/Parameters.hh"  // set edit mode
#include "pds/config/DbClient.hh"

#include <QtGui/QApplication>

#include <stdio.h>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  bool edit=false;
  bool dbnamed=false;
  string dbname;
  bool lusage=false;

  for(int i=1; i<argc; i++) {
    if (strcmp(argv[i],"--db")==0) {
      dbname = string(argv[++i]);
      dbnamed = true;
    }
    else lusage=true;
  }
  lusage |= !dbnamed;
  if (lusage) {
    printf("%s --db <dbname>\n",argv[0]);
    return 1;
  }

  QApplication app(argc, argv);

  DbClient* db = DbClient::open(dbname.c_str());
  if (!db) {
    printf("Database root %s is not valid\n",dbname.c_str());
    if (!edit) return -1;
  }

  Parameter::allowEdit(edit);

  ListUi* ui = new ListUi(*db);
  ui->show();
  app.exec();

  //  printf("\n==AFTER==\n");
  //  db.dump();
}
