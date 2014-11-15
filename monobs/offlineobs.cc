#include "pds/service/CmdLineTools.hh"
#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/offlineclient/OfflineClient.hh"
#include "OfflineAppliance.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

static const char *helpText =
  "PV Config File Format:\n"
  "  - Each line of the file can contain one PV name\n"
  "  - Use '#' at the beginning of the line to comment out whole line\n"
  "  - Use '#' in the middle of the line to comment out the remaining characters\n"
  "  - Use '*' at the beginning of the line to define an alias for the immediately following PV(s)\n"
  "  - Use '<' to include file(s)\n\n"
  "Example:\n"
  "  % cat logbook.txt\n"
  "  < PvList0.txt, PvList1.txt # Include Two Files\n"
  "  iocTest:aiExample          # PV Name\n"
  "  # This is a comment line\n"
  "  iocTest:calcExample1\n"
  "  * electron beam energy     # Alias for BEND:DMP1:400:BDES\n"
  "  BEND:DMP1:400:BDES\n";

using namespace Pds;

class MyCallback : public EventCallback {
public:
  MyCallback(Task* task, Appliance* app) :
    _task(task),
    _app(app)
  {
  }
  ~MyCallback() {}

  void attached (SetOfStreams& streams)
  {
    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _app->connect(frmk->inlet());
  }
  void failed   (Reason reason)   { _task->destroy(); delete this; }
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  Appliance*  _app;
};

void usage(char* progname) {
  printf("Usage: %s -p <platform> -P <partition> -L <offlinerc> [-E <experiment_name>] [-V <pv_config_file>] [OPTIONS]\n", progname);
  printf("\nOptions:\n");
  printf("  -g        Use %%g format for floating point PVs\n");
  printf("  -h        Print help message and exit\n");
  printf("  -v        Be verbose\n");
  printf("  -w        Slow readout (0 or 1, default=0)\n");
}

// Appliance* app;
OfflineAppliance* app;
OfflineClient* offlineclient;

void sigfunc(int sig_no) {
  printf("caught signal %d\n", sig_no);
  if (app) {
    delete app;
    app = 0;
    printf("deleted app\n");
  }
  exit(EXIT_SUCCESS);
}

void exit_failure() {
  delete app;
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {

  unsigned platform=-1U;
  const char* partition = 0;
  const char* experiment_name = 0;
  const char* offlinerc = 0;
  const char* parm_list_file = 0;
  unsigned nodes =  0;
  int      slowReadout = 0;
  int verbose = 0;
  (void) signal(SIGINT, sigfunc);
  bool parseErr = false;
  bool gFormat = false;
  extern int optind;
  int c;
  while ((c = getopt(argc, argv, "p:P:E:L:V:w:vgh")) != -1) {
    errno = 0;
    switch (c) {
    case 'p':
      if (!CmdLineTools::parseUInt(optarg, platform)) {
        printf("%s: failed to parse platform '%s'\n", argv[0], optarg);
        parseErr = true;
      }
      break;
    case 'P':
      partition = optarg;
      break;
    case 'E':
      experiment_name = optarg;
      break;
    case 'L':
      offlinerc = optarg;
      break;
    case 'V':
      parm_list_file = optarg;
      break;
    case 'v':
      ++verbose;
      break;
    case 'g':
      printf("%s: using %%g format for floating point PVs\n", argv[0]);
      gFormat = true;
      break;
    case 'h':
      usage(argv[0]);
      printf("\n%s", helpText);
      return 0;
    case 'w':
      if (!CmdLineTools::parseInt(optarg, slowReadout)) {
        parseErr = true;
      }
      if ((slowReadout != 0) && (slowReadout != 1)) {
        parseErr = true;
      }
      break;
    case '?':
    default:
      parseErr = true;
      break;
    }
  }

  if (platform == -1U) {
    printf("%s: platform is required\n", argv[0]);
    parseErr = true;
  }

  if (!offlinerc) {
    printf("%s: offlinerc is required\n", argv[0]);
    parseErr = true;
  }

  if (!partition) {
    printf("%s: partition is required\n", argv[0]);
    parseErr = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    parseErr = true;
  }

  if (parseErr) {
    usage(argv[0]);
    return 1;
  }

  // Parse instrument name for optional station number.
  // Examples: "AMO", "SXR:0", "CXI:0", "CXI:1"
  PartitionDescriptor pd(partition);
  if (!pd.valid()) {
    fprintf(stderr, "%s: Error parsing partition '%s'\n", argv[0], partition);
    usage(argv[0]);
    return 1;
  }
  if (experiment_name) {
    offlineclient = new OfflineClient(offlinerc, pd, experiment_name, (verbose > 0));

    unsigned expnum = offlineclient->GetExperimentNumber();
    if (expnum != OFFLINECLIENT_DEFAULT_EXPNUM) {
      printf("%s: instrument %s:%u experiment %s (#%u)\n", argv[0],
             pd.GetInstrumentName().c_str(), pd.GetStationNumber(), offlineclient->GetExperimentName(), expnum);
      app = new OfflineAppliance(offlineclient, parm_list_file, OFFLINECLIENT_MAX_PARMS, (verbose > 0), gFormat);
    } else {
      fprintf(stderr, "%s: failed to find experiment '%s'\n", argv[0], experiment_name);
      app = NULL;
    }
  } else {
    offlineclient = new OfflineClient(offlinerc, pd, (verbose > 0));

    const char *expname = offlineclient->GetExperimentName();
    if (expname) {
      printf("%s: instrument %s:%u experiment %s (#%u)\n", argv[0],
             pd.GetInstrumentName().c_str(), pd.GetStationNumber(), expname, offlineclient->GetExperimentNumber());
      app = new OfflineAppliance(offlineclient, parm_list_file, OFFLINECLIENT_MAX_PARMS, (verbose > 0), gFormat);
    } else {
      fprintf(stderr, "%s: failed to find current experiment\n", argv[0]);
      app = NULL;
    }
  }
  if (app) {
    Task* task = new Task(Task::MakeThisATask);
    MyCallback* display = new MyCallback(task, app);

    ObserverLevel* event = new ObserverLevel(platform,
               partition,
               nodes,
               *display,
               slowReadout,
               0 // max event size = default
               );

    if (event->attach()) {
      if (verbose) {
        printf("%s: Observer attached, entering main loop...\n", argv[0]);
      }
      task->mainLoop();
      event->detach();
      delete app;
      delete event;
      delete display;
    } else {
      fprintf(stderr, "%s: Observer failed to attach to platform\n", argv[0]);
      delete app;
      delete event;
    }
  } else {
    fprintf(stderr, "%s: Error creating OfflineAppliance\n", argv[0]);
  }
  return 0;
}

