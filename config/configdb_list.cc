#include "pdsapp/config/ListUi.hh"
#include "pdsapp/config/Parameters.hh"  // set edit mode
#include "pds/config/DbClient.hh"

#include <QtGui/QApplication>

#include <stdio.h>
#include <getopt.h>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  bool edit=false;
  bool dbnamed=false;
  string dbname;
  bool lusage=false;

  int c;
  while (1) {
    static struct option long_options[] = {
      {"db", 1, 0, 'd'},
      {"help", 0, 0, 'h'},
      {0, 0, 0, 0}
    };
    c = getopt_long(argc, argv, "h", long_options, NULL);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'd':
        if (optarg) {
          if (*optarg == '-') {
            lusage = true;  // error: --db followed by another option
          } else {
            dbname = optarg;
            dbnamed = true;
          }
        }
        break;
      case 'h':
        lusage = true;
        break;
      default:
      case '?':
        // error
        lusage = true;
     }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lusage = true;
  }

  if (lusage || !dbnamed) {
    printf("Usage: %s --db <path> [-h]\n",argv[0]);
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
