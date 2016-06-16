#include "pdsapp/control/MainWindow.hh"
#include "pdsapp/control/SelectDialog.hh"
#include "pdsapp/control/EventcodeQuery.hh"
#include "pdsapp/config/Experiment.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/CmdLineTools.hh"

#include <QtGui/QApplication>

#include <stdio.h>
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <climits>

extern int optind;

using namespace Pds;
using Pds_ConfigDb::Experiment;

static void usage(char *argv0)
{
  printf("Usage: %s -p <platform> -P <partition_description> -D <db name> [options]\n"
   "Options: -L <offlinerc>            : offline db access\n"
   "         -E <experiment_name>      : offline db experiment\n"
   "         -R <run_number_file>      : no offline db\n"
   "         -e <experiment_number>    : no offline db experiment number\n"
   "         -N <seconds>              : log long NFS accesses\n"
   "         -C <controls_config_file> : configuration of controls recorder\n"
   "         -A                        : auto run\n"
   "         -O                        : override errors\n"
   "         -T                        : collect transient data\n"
   "         -t <interval>             : traffic shaping interval[sec]\n"
   "         -w <0/1>                  : slow readout\n"
   "         -o <options>              : partition options\n"
   "            1=CXI slow runningkludge\n"
   "            2=XPP short timeout on Disable\n"
   "         -X <host>:<port>          : status export host name and UDP port number\n"
   "         -I <ignore_options>       : ignore PV connection errors for IOC recorder\n"
   "            0=Do not ignore (default)\n"
   "            1=Ignore with message\n"
   "            2=Ignore silently\n"
   "         -h                        : print usage information\n"
   "         -v\n",
   argv0);
}

int main(int argc, char** argv)
{
  const unsigned NO_PLATFORM = UINT_MAX;
  unsigned platform = NO_PLATFORM;
  const char* partition = (char *)NULL;
  const char* dbpath    = (char *)NULL;
  const char* offlinerc = (char *)NULL;
  const char* runNumberFile = (char *)NULL;
  const char* experiment = (char *)NULL;
  const char* controlrc = (char *)NULL;
  double nfs_log_threshold = -1;
  unsigned    sequencer_id = 0;
  unsigned    expnum = 0;
  const char* status_host_and_port = (char *)NULL;
  int         slowReadout = 0;
  unsigned key=0;
  int verbose = 0;
  bool override = false;
  unsigned partition_options = 0;
  unsigned pv_ignore_options = 0;
  bool autorun = false;
  bool lusage = false;

  int c;
  while ((c = getopt(argc, argv, "p:P:D:L:R:E:e:N:C:AOTS:X:t:w:o:I:hv")) != -1) {
    switch (c) {
    case 'A':
      autorun = true;
      break;
    case 'p':
      if (!Pds::CmdLineTools::parseUInt(optarg, platform)) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'P':
      partition = optarg;
      break;
    case 'D':
      dbpath = optarg;
      break;
    case 'k':
      if (!Pds::CmdLineTools::parseUInt(optarg, key)) {
        printf("%s: option `-k' parsing error\n", argv[0]);
        lusage = true;
      }
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
    case 'e':
      if (!Pds::CmdLineTools::parseUInt(optarg, expnum)) {
        printf("%s: option `-e' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'N':
      if (!Pds::CmdLineTools::parseDouble(optarg, nfs_log_threshold)) {
        printf("%s: option `-N' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'C':
      controlrc = optarg;
      break;
    case 'O':
      override = true;
      break;
    case 'S':
      if (!Pds::CmdLineTools::parseUInt(optarg, sequencer_id)) {
        printf("%s: option `-S' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'X':
      if (strchr(optarg, ':')) {
        status_host_and_port = optarg;
      } else {
        printf("%s: option `-X' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'T':
      SelectDialog::useTransient(true);
      break;
    case 't':
      Allocation::set_traffic_interval(strtod(optarg,NULL));
      break;
    case 'w':
      if (!Pds::CmdLineTools::parseInt(optarg, slowReadout)) {
        printf("%s: option `-w' parsing error\n", argv[0]);
        lusage = true;
      } else if ((slowReadout != 0) && (slowReadout != 1)) {
        printf("%s: option `-w' out of range\n", argv[0]);
        lusage = true;
      }
      break;
    case 'o':
      if (!Pds::CmdLineTools::parseUInt(optarg, partition_options)) {
        printf("%s: option `-o' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'I':
      if (!Pds::CmdLineTools::parseUInt(optarg, pv_ignore_options)) {
        printf("%s: option `-I' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'h':
      lusage = true;
      break;
    case 'v':
      ++verbose;
      break;
    case '?':
    default:
      // error
      lusage = true;
      break;
    }
  }
  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lusage = true;
  }
  if (platform==NO_PLATFORM) {
    printf("%s: platform is required\n", argv[0]);
    lusage = true;
  }
  if (!partition) {
    printf("%s: partition is required\n", argv[0]);
    lusage = true;
  }
  if (!dbpath) {
    printf("%s: db path is required\n", argv[0]);
    lusage = true;
  }
  if (!offlinerc && experiment) {
    printf("%s: offlinerc is required with experiment name\n", argv[0]);
    lusage = true;
  }
  if (offlinerc && runNumberFile) {
    printf("%s: offlinerc and run number file are mutually exclusive\n", argv[0]);
    lusage = true;
  }
  if (lusage) {
    usage(argv[0]);
    return 0;
  }

  Experiment::log_threshold(nfs_log_threshold);

  //  EPICS thread initialization
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), 
           "control calling ca_context_create" );
  EventcodeQuery::execute();

  int _argc=1;
  const char* _argv[] = { "DAQ Control", NULL };
  QApplication app(_argc, const_cast<char**>(_argv));

  MainWindow* window = new MainWindow(platform,
                                      partition,
                                      dbpath,
                                      offlinerc,
                                      runNumberFile,
                                      experiment,
                                      sequencer_id,
                                      slowReadout,
                                      partition_options,
                                      (verbose > 0),
                                      controlrc,
                                      expnum,
                                      status_host_and_port,
                                      pv_ignore_options);
  window->override_errors(override);
  window->show();
  if (autorun)
    window->autorun();

  app.exec();

  ca_context_destroy();

  return 0;
}
