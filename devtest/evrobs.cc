#include "pds/collection/CollectionManager.hh"
#include "pds/service/Task.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvrManager.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

namespace Pds {

  //
  //  Implementation of the transition observer 
  //    (sees the entire platform)
  //
  static const unsigned MaxPayload = sizeof(Allocate);
  static const unsigned ConnectTimeOut = 250; // 1/4 second

  class Observer : public CollectionManager {
  public:
    Observer(unsigned char platform) :
      CollectionManager(Level::Observer, platform, MaxPayload, ConnectTimeOut, NULL) {}
    ~Observer() {}
  public:
    virtual void post(const Transition&) = 0;
  private:
    void message(const Node& hdr, const Message& msg)
    {
      if (hdr.level() == Level::Control && msg.type()==Message::Transition) {
        const Transition& tr = reinterpret_cast<const Transition&>(msg);
	if (tr.phase() == Transition::Execute)
	  post(tr);
      }
    }
  };

  class EvrObserver : public Observer {
  public:
    EvrObserver(const char* dev, unsigned platform) : 
      Observer(platform), 
      _info   (dev),
      _mgr    (_info, EvgrOpcode::L1Accept) {}
    void post(const Transition& tr) {
      Transition* ptr = const_cast<Transition*>(&tr);
      printf("EvrObserver::post tr id %d\n",tr.id());
      _mgr.appliance().transitions(ptr);
    }
  private:
    EvgrBoardInfo<Evr> _info;
    EvrManager         _mgr;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = 0;
  const char* dev = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "p:d:")) != EOF ) {
    switch(c) {
    case 'd':
      dev = optarg;
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    }
  }

  if (!platform) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }
  if (!dev) {
    printf("%s: dev required\n",argv[0]);
    return 0;
  }

  EvrObserver observer(dev, platform);
  observer.start();
  observer.connect();

  Task* task = new Task(Task::MakeThisATask);
  task->mainLoop();

  return 0;
}
