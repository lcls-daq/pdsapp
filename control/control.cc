#include "pdsapp/control/MainWindow.hh"
#include "pdsapp/config/Experiment.hh"

#include <QtGui/QApplication>

#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>

using namespace Pds;
using Pds_ConfigDb::Experiment;

static void usage(char *argv0)
{
  printf("usage: %s -p <platform> -P <partition_description> -D <db name> [options]\n"
	 "Options: -L <offlinerc>        : offline db access\n"
	 "         -E <experiment_name>  : offline db experiment\n"
	 "         -R <run_number_file>  : no offline db\n"
	 "         -N <seconds>          : log long NFS accesses\n"
	 "         -O                    : override errors\n"
	 "         -v\n", 
	 argv0);
}

int main(int argc, char** argv)
{
  const unsigned NO_PLATFORM = (unsigned)-1;
  unsigned platform = NO_PLATFORM;
  const char* partition = "partition";
  const char* dbpath    = "none";
  const char* offlinerc = (char *)NULL;
  const char* runNumberFile = (char *)NULL;
  const char* experiment = (char *)NULL;
  double nfs_log_threshold = -1;
  unsigned    sequencer_id = 0;
  unsigned key=0;
  int verbose = 0;
  bool override = false;

  int c;
  while ((c = getopt(argc, argv, "p:P:D:L:R:E:N:OS:v")) != -1) {
    char* endPtr;
    switch (c) {
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
    case 'L':
      offlinerc = optarg;
      break;
    case 'R':
      runNumberFile = optarg;
      break;
    case 'E':
      experiment = optarg;
      break;
    case 'N':
      nfs_log_threshold = strtod(optarg, NULL);
      break;
    case 'O':
      override = true;
      break;
    case 'S':
      sequencer_id = strtoul(optarg, &endPtr, 0);
      break;
    case 'v':
      ++verbose;
      break;
    }
  }
  if ((platform==NO_PLATFORM || !partition || !dbpath) || (!offlinerc && experiment) || (offlinerc && runNumberFile)) {
    usage(argv[0]);
    return 0;
  }

  Experiment::log_threshold(nfs_log_threshold);

  int _argc=1;
  char* _argv[] = { "DAQ Control", NULL };
  QApplication app(_argc, _argv);
  
  MainWindow* window = new MainWindow(platform,
                                      partition,
                                      dbpath,
                                      offlinerc,
                                      runNumberFile,
                                      experiment,
                                      sequencer_id,
                                      (verbose > 0));
  window->override_errors(override);
  window->show();
  app.exec();

  return 0;
}
