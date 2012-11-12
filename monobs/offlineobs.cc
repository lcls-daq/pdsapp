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
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

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
  printf("Usage: %s -p <platform> -P <partition> -L <offlinerc> [-V <parm_list_file>] [-v]\n", progname);
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

  unsigned platform=-1UL;
  const char* partition = 0;
  const char* offlinerc = 0;
  const char* parm_list_file = 0;
  unsigned nodes =  0;
  int verbose = 0;
  (void) signal(SIGINT, sigfunc);
  int c;
  while ((c = getopt(argc, argv, "p:P:E:L:V:v")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = -1UL;
      break;
    case 'P':
      partition = optarg;
      break;
    case 'E':
      printf("%s: -E flag is ignored, current experiment will be used\n", argv[0]);
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
    default:
      break;
    }
  }

  if (platform == -1UL || !partition || !offlinerc) {
    fprintf(stderr, "Missing parameters!\n");
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
  offlineclient = new OfflineClient(offlinerc, pd, (verbose > 0));

  const char *expname = offlineclient->GetExperimentName();
  if (expname) {
    printf("%s: instrument %s:%u experiment %s (#%u)\n", argv[0],
           pd.GetInstrumentName().c_str(), pd.GetStationNumber(), expname, offlineclient->GetExperimentNumber());
    app = new OfflineAppliance(offlineclient, parm_list_file);
  } else {
    fprintf(stderr, "%s: failed to find current experiment\n", argv[0]);
    app = NULL;
  }
  if (app) {
    Task* task = new Task(Task::MakeThisATask);
    MyCallback* display = new MyCallback(task, app);

    ObserverLevel* event = new ObserverLevel(platform,
               partition,
               nodes,
               *display);

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

