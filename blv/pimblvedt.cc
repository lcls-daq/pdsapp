#include "pdsapp/blv/IdleStream.hh"
#include "pdsapp/blv/ShmOutlet.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/camera/PimManager.hh"
#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/FrameServer.hh"
#include "pds/camera/EdtPdvCL.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <time.h>

static Pds::CameraDriver* _driver(int id)
{
  return new Pds::EdtPdvCL(*new Pds::TM6740Camera, 0, id);
}

static void *thread_signals(void*)
{
  while(1) sleep(100);
  return 0;
}

using namespace Pds;

class MyTimeStamp : public Appliance {
public:
  Transition* transitions(Transition* in) { return in; }
  InDatagram* events     (InDatagram* in) { 
    timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    unsigned pulseId = -1;
    ClockTime clocktime(tp.tv_sec, tp.tv_nsec);
    TimeStamp timestamp(0, pulseId, 0);
    in->datagram().seq = Sequence(Sequence::Event, in->datagram().seq.service(), clocktime, timestamp);
    return in;
  }
};


int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  unsigned grabberId(0);
  unsigned controlPort=1100;
  const char* shmtag = 0;
  unsigned shmbuffersize = 0x100000;
  unsigned shmbuffers = 16;
  unsigned shmclients = 1;

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "i:g:c:t:")) != EOF ) {
    switch(c) {
    case 'i':
      det    = (DetInfo::Detector)strtoul(optarg, &endPtr, 0);
      detid  = strtoul(endPtr+1, &endPtr, 0);
      devid  = strtoul(endPtr+1, &endPtr, 0);
      break;
    case 'g':
      grabberId = strtoul(optarg, &endPtr, 0);
      break;
    case 'c':
      controlPort = strtoul(optarg, NULL, 0);
      break;
    case 't':
      shmtag = optarg;
      break;
    }
  }

  printf("Starting handler thread ...\n");
  pthread_t h_thread_signals;
  pthread_create(&h_thread_signals, NULL, thread_signals, 0);

  //
  // Block all signals from this thread (and daughters)
  //
  sigset_t sigset_full;
  sigfillset(&sigset_full);
  pthread_sigmask(SIG_BLOCK, &sigset_full, 0);

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,0);
  DetInfo src(node.pid(), 
	      det, detid, 
	      DetInfo::TM6740, devid);

  //
  //  Setup the idle stream
  //
  ProcInfo idleSrc(Level::Segment,0,0);
  IdleStream* idle = new IdleStream(controlPort, 
				    idleSrc);
  ShmOutlet* outlet = new ShmOutlet(*idle->outlet(),
				    shmtag, shmbuffersize, shmbuffers, shmclients);

  //  Add an appliance to set the event time
  (new MyTimeStamp)->connect(idle->inlet());

  PimManager& icamman = *new PimManager(src);
  icamman.appliance().connect(idle->inlet());
  icamman.attach(_driver(grabberId));

  //  create the inlet wire/event builder
  const int MaxSize = 0x100000;
  const int ebdepth = 4;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *idle->inlet(),
					*outlet, 0, idle->ip(), MaxSize, ebdepth, 0);
  iwire->connect();
  //  attach the server
  iwire->add_input(&icamman.server());
  idle->set_inlet_wire(iwire);
  idle->start();

  task->mainLoop();

  icamman.detach();

  return 0;
}
