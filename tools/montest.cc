#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"

#include "pds/utility/Appliance.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/camera/Frame.hh"

#include "pds/mon/MonServerManager.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescImage.hh"
#include "pds/mon/MonEntryImage.hh"

#include "pds/service/Semaphore.hh"

#include <time.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

using namespace Pds;

class CamDisplay : public Appliance, XtcIterator {
  enum { Columns=1024 };
  enum { Rows=1024 };
  enum { BinShift=1 };
public:
  CamDisplay(unsigned detectorId,
       MonServerManager& monsrv) :
    _detectorId(detectorId),
    _iter      (sizeof(ZcpDatagramIterator),1),
    _monsrv    (monsrv)
  {
    MonGroup* group = new MonGroup("Image Group");
    monsrv.cds().add(group);
    MonDescImage desc("Image",Columns>>BinShift,Rows>>BinShift,1<<BinShift,1<<BinShift);
    group->add(_entry = new MonEntryImage(desc));
  }
  ~CamDisplay()
  {
  }

  int process(const Xtc& xtc,
        InDatagramIterator* iter)
  {
    if (xtc.contains.id()==TypeId::Id_Xtc)
      return iterate(xtc,iter);

    int advance = 0;
    if (xtc.src.phy() == _detectorId && xtc.contains.id() == TypeId::Id_Frame) {
      Pds::Semaphore& sem = _monsrv.cds().payload_sem();
      sem.take();  // make image update atomic

      //  copy the frame header
      Frame frame;
      advance += iter->copy(&frame, sizeof(Frame));

      unsigned remaining = frame.data_size();
      iovec iov;
      unsigned ix=0,iy=0;
      //  Ignoring the possibility of fragmenting on an odd-byte
      while(remaining) {
  int len = iter->read(&iov,1,remaining);
  remaining -= len;
  const unsigned short* w = (const unsigned short*)iov.iov_base;
  const unsigned short* end = w + (len>>1);
  while(w < end) {
    _entry->addcontent(*w++,(ix++)>>BinShift,iy>>BinShift);
    if (ix==frame.width()) { ix=0; iy++; }
  }
      }
      _entry->time(_now);

      sem.give();  // make image update atomic
    }
    return advance;
  }

  Transition* transitions(Transition* in) { return in; }
  InDatagram* occurrences(InDatagram* in) { return in; }
  InDatagram* events     (InDatagram* in)
  {
    if (in->datagram().seq.service() == TransitionId::L1Accept) {
      _now = in->datagram().seq.clock();
      InDatagramIterator* in_iter = in->iterator(&_iter);
      iterate(in->datagram().xtc,in_iter);
      delete in_iter;
    }
    return in;
  }
private:
  unsigned          _detectorId;
  GenericPool       _iter;
  MonServerManager& _monsrv;
  ClockTime         _now;
  MonEntryImage*    _entry;
};

class MyCallback : public EventCallback {
public:
  MyCallback(Task* task, const DetInfo& detInfo, MonServerManager& monsrv) :
    _task(task),
    _display(new CamDisplay(detInfo.phy(),monsrv))
  {}
  ~MyCallback() {}

  void attached (SetOfStreams& streams)
  {
    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _display->connect(frmk->inlet());
  }
  void failed   (Reason reason) { _task->destroy(); delete this; }
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  CamDisplay* _display;
};

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = (unsigned)-1;
  unsigned platform=NO_PLATFORM;
  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  const char* arpsuidprocess = 0;
  const char* partition = 0;
  unsigned    nodes = 0;
  int         slowReadout = 0;
  int c;
  while ((c = getopt(argc, argv, "d:p:a:i:P:")) != -1) {
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
    case 'w':
      slowReadout = strtoul(optarg, &endPtr, 0);
      break;
    case 'P':
      partition = optarg;
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
  Task* task = new Task(Task::MakeThisATask);
  MyCallback* display = new MyCallback(task,
               DetInfo(0,
                 det, detid,
                 DetInfo::Opal1000, devid),
               *manager);
  manager->serve();

  ObserverLevel* event = new ObserverLevel(platform,
             partition,
             nodes,
             *display,
             slowReadout,
             0 // max event size = default
             );

  if (event->attach())
    task->mainLoop();

  event->detach();

  manager->dontserve();
  delete manager;
  delete display;
  delete event;
  if (arp) delete arp;
  return 0;
}

