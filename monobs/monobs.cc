#include "CamDisplay.hh"
#include "AcqDisplay.hh"

#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/mon/MonServerManager.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

using namespace Pds;

class MyCallback : public EventCallback {
public:
  MyCallback(Task* task, const DetInfo& detInfo, MonServerManager& monsrv) : 
    _task(task), 
    _camdisp(new CamDisplay("Cam",detInfo.phy(),monsrv)) 
  {
    DisplayConfig& dc = *new DisplayConfig("Acqiris Group");
    DetInfo det(0, DetInfo::AmoIms, 0, DetInfo::Acqiris, 0);
    dc.request(det);
    _acqdisp = new AcqDisplay(dc);
  }
  ~MyCallback() {}

  void attached (SetOfStreams& streams) 
  {
    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _camdisp->connect(frmk->inlet());
    _acqdisp->connect(frmk->inlet());
  }
  void failed   (Reason reason) { _task->destroy(); delete this; }
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  CamDisplay* _camdisp;
  AcqDisplay* _acqdisp;
};

int main(int argc, char** argv) {

  unsigned platform=-1UL;
  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  const char* arpsuidprocess = 0;
  const char* partition = 0;
  unsigned node = 0;
  int c;
  while ((c = getopt(argc, argv, "d:p:a:i:P:")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = -1UL;
      break;
    case 'a':
      arpsuidprocess = optarg;
      break;
    case 'd':
      det    = (DetInfo::Detector)strtoul(optarg, &endPtr, 0);
      detid  = strtoul(endPtr, &endPtr, 0);
      devid  = strtoul(endPtr, &endPtr, 0);
      break;
    case 'i':
      node = strtoul(optarg, &endPtr, 0);
      break;
    case 'P':
      partition = optarg;
      break;
    default:
      break;
    }
  }

  if (platform == -1UL) {
    printf("Platform required\n");
    printf("Usage: %s -p <platform> -P <partition> -d <detector_id> -i <monitor node> [-a <arp process id>]\n",
	   argv[0]);
    return 1;
  }

  Arp* arp=0;
  if (arpsuidprocess) {
    arp = new Arp(arpsuidprocess);
    if (arp->error()) {
      char message[128];
      sprintf(message, "failed to create Arp for `%s': %s", 
	      arpsuidprocess, strerror(arp->error()));
      printf("%s: %s\n", argv[0], message);
      delete arp;
      return 0;
    }
  }

  MonServerManager* manager = new MonServerManager(MonPort::Test);
  Task* task = new Task(Task::MakeThisATask);
  MyCallback* display = new MyCallback(task, 
				       DetInfo(0,
					       det, detid,
					       DetInfo::Opal1000, devid),
				       *manager);
  manager->serve();

  ObserverLevel* event = new ObserverLevel(platform,
					   partition,
					   node,
					   *display);

  if (event->attach())
    task->mainLoop();
  else
    printf("Observer failed to attach to platform\n");

  event->detach();

  manager->dontserve();
  delete manager;
  delete display;
  delete event;
  if (arp) delete arp;
  return 0;
}

