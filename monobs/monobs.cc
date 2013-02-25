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

class Prescaler : public Appliance {
public:
  Prescaler(unsigned prescale) : _prescale(prescale), _count(0) {}
public:
  Transition* transitions(Transition* tr) { return tr; }
  InDatagram* occurrences(InDatagram* dg) { return dg; }
  InDatagram* events     (InDatagram* dg)
  { if (dg->datagram().seq.isEvent())
      if (++_count == _prescale) {
  _count = 0;
  return dg;
      }
      else
  return 0;
    return dg;
  }
private:
  unsigned _prescale;
  unsigned _count;
};

class MyCallback : public EventCallback {
public:
  MyCallback(Task* task, Appliance* app) :
    _task(task),
    _appliances(app)
  {
  }
  ~MyCallback() {}

  void attached (SetOfStreams& streams)
  {
    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _appliances->connect(frmk->inlet());
  }
  void failed   (Reason reason)   { _task->destroy(); delete this; }
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  Appliance*  _appliances;
};

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = (unsigned)-1;
  unsigned platform=NO_PLATFORM;
  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);
  unsigned prescale=0;

  const char* arpsuidprocess = 0;
  const char* partition = 0;
  unsigned nodes = 0;
  int      slowReadout = 0;
  int c;
  while ((c = getopt(argc, argv, "d:p:a:i:P:s:w:")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = NO_PLATFORM;
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
      nodes = strtoul(optarg, &endPtr, 0);
      break;
    case 'P':
      partition = optarg;
      break;
    case 's':
      prescale = strtoul(optarg, &endPtr, 0);
      break;
    case 'w':
      slowReadout = strtoul(optarg, &endPtr, 0);
      break;
    default:
      break;
    }
  }

  if (platform == NO_PLATFORM) {
    printf("Platform required\n");
    printf("Usage: %s -p <platform> -P <partition> -d <detector_id> -i <node mask> [-a <arp process id>]\n",
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

  MonServerManager* manager = new MonServerManager(MonPort::Mon);
  CamDisplay* camdisp = new CamDisplay(*manager);

  AcqDisplay* acqdisp = new AcqDisplay(*manager);
  manager->serve();

  Appliance* apps;
  if (prescale) {
    apps = new Prescaler(prescale);
    camdisp->connect(apps);
  }
  else {
    apps = camdisp;
  }
  acqdisp->connect(apps);

  Task* task = new Task(Task::MakeThisATask);
  MyCallback* display = new MyCallback(task,
               apps);

  ObserverLevel* event = new ObserverLevel(platform,
             partition,
             nodes,
             *display,
             slowReadout,
             0 // max event size = default
             );

  if (event->attach())
    task->mainLoop();
  else
    printf("Observer failed to attach to platform\n");

  event->detach();

  manager->dontserve();
  delete event;
  delete display;
  delete camdisp;
  delete acqdisp;
  delete manager;
  if (arp) delete arp;
  return 0;
}

