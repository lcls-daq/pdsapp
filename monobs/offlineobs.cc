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
  printf("Usage: %s -p <platform> -P <partition> -E <experiment_name> -L <offlinerc> [-V <parm_list_file>]\n", progname);
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
  const char* experiment_name = 0;
  const char* offlinerc = 0;
  const char* parm_list_file = 0;
  unsigned nodes =  0;
  (void) signal(SIGINT, sigfunc);
  int c;
  while ((c = getopt(argc, argv, "p:P:E:L:V:")) != -1) {
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
      experiment_name = optarg;
      break;
    case 'L':
      offlinerc = optarg;
      break;
    case 'V':
      parm_list_file = optarg;
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

  if (!experiment_name) {
    offlineclient = new OfflineClient(offlinerc, partition);
  }
  else {
    offlineclient = new OfflineClient(offlinerc, partition, experiment_name);
  }

  app = new OfflineAppliance(offlineclient, parm_list_file);
  if (app) {
    Task* task = new Task(Task::MakeThisATask);
    MyCallback* display = new MyCallback(task, app);

    ObserverLevel* event = new ObserverLevel(platform,
               partition,
               nodes,
               *display);

    if (event->attach()) {
      task->mainLoop();
      event->detach();
      delete app;
      delete event;
      delete display;
    } else {
      printf("Observer failed to attach to platform\n");
      delete app;
      delete event;
    }
  } else {
    printf("%s: Error creating OfflineAppliance\n", __FUNCTION__);
  }
  return 0;
}

