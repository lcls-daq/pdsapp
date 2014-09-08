#include "pds/service/CmdLineTools.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/collection/CollectionServer.hh"
#include "pds/utility/Transition.hh"
#include "pds/collection/CollectionPorts.hh"
#include "pds/collection/Route.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Client.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/service/Task.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <getopt.h>

using namespace Pds;

void usage(const char* p)
{
  printf("Usage: %s <platform> [-h]\n", p);
}

class EvrService: public CollectionManager {
public:
  EvrService(unsigned);
  virtual ~EvrService();

private:
  // Implements CollectionManager
  virtual void message(const Node& hdr, const Message& msg);

private:
  Client   _outlet;
  unsigned _evr;
};


static const unsigned MaxPayload = sizeof(Allocate);
static const unsigned ConnectTimeOut = 500; // 1/2 second

EvrService::EvrService(unsigned platform) :
  CollectionManager(Level::Observer, platform,
                    MaxPayload, ConnectTimeOut, NULL),
  _outlet(sizeof(EvrDatagram),0),
  _evr   (0)
{}

EvrService::~EvrService()
{
}

void EvrService::message(const Node& hdr, const Message& msg)
{
  if (hdr.level() == Level::Control) {
    if (msg.type() == Message::Transition) {
      const Transition& tr = reinterpret_cast<const Transition&>(msg);
      if (tr.id() == TransitionId::L1Accept &&
    tr.phase() == Transition::Record) {
  EvrDatagram datagram(tr.sequence(), _evr++);
  Ins dst(StreamPorts::event(hdr.platform(),
              Level::Segment));
  _outlet.send((char*)&datagram,(char*)0,0,dst);
  if (_evr%1000 == 0)
    printf("EvrService::out %x:%08x/%08x to %x/%d\n",
     datagram.evr,
     datagram.seq.stamp().fiducials(),datagram.seq.stamp().ticks(),
     dst.address(),dst.portId());
      }
      else {
  printf("Resetting evr count @ 0x%x (%d)\n",_evr,_evr);
  _evr = 0;  // reset the sequence on any transition
      }
    }
  }
}

int main(int argc, char** argv)
{
  unsigned platform;
  bool parseErr = false;
  int c;
  while ((c = getopt(argc, argv, "h")) != -1) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      exit(0);
    case '?':
    default:
      parseErr = true;
      break;
    }
  }

  if (parseErr || (argc != 2) || !CmdLineTools::parseUInt(argv[1], platform)) {
    usage(argv[0]);
    exit(1);
  }

  platform &= 0xff;
  EvrService evr(platform);

  evr.start();
  evr.connect();

  Task* task = new Task(Task::MakeThisATask);
  task->mainLoop();

  return 0;
}
