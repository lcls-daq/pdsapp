#include "pdsapp/control/MainWindow.hh"
#include "pdsapp/config/Experiment.hh"

#include <QtGui/QApplication>

#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>

using namespace Pds;
using Pds_ConfigDb::Experiment;

int main(int argc, char** argv)
{
  unsigned platform = -1UL;
  unsigned bldList[32];
  unsigned nbld = 0;
  const char* partition = "partition";
  const char* dbpath    = "none";
  unsigned key=0;

  int c;
  while ((c = getopt(argc, argv, "p:b:P:D:")) != -1) {
    char* endPtr;
    switch (c) {
    case 'b':
      bldList[nbld++] = strtoul(optarg, &endPtr, 0);
      break;
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = 0;
      break;
    case 'P':
      partition = optarg;
      break;
    case 'D':
      dbpath = optarg;
      break;
    case 'k':
      key = strtoul(optarg, &endPtr, 0);
      break;
    }
  }
  if (platform==-1UL || !partition || !dbpath) {
    printf("usage: %s -p <platform> -P <partition_description> -D <db name> [-b <bld>]\n", argv[0]);
    return 0;
  }

  QApplication app(argc, argv);

  MainWindow* window = new MainWindow(platform,
				      partition,
				      dbpath);
  window->show();
  app.exec();

  return 0;
}
