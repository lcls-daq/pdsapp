#include "CamDisplay.hh"
#include "AcqDisplay.hh"
#include "XtcMonitorClient.hh"

#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/mon/MonServerManager.hh"

#include "pds/utility/Appliance.hh"
#include "pds/xtc/CDatagramIterator.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

using namespace Pds;

class MyInlet : public Appliance {
public:
  MyInlet() {}
  ~MyInlet() {}
public:
  Transition* transitions(Transition*) { return 0; }
  InDatagram* events     (InDatagram*) { return 0; }
};

class MyDatagram : public InDatagram {
public:
  MyDatagram(Dgram& dg) : _dg(reinterpret_cast<Datagram&>(dg)) {}
  ~MyDatagram() {}
public:
  void* operator new(size_t size, Pds::Pool* pool) { return pool->alloc(size); }
  void  operator delete(void* p) { Pds::RingPool::free(p); }
public:
  const Datagram& datagram() const { return _dg; }
  Datagram& datagram() { return _dg; }

  bool insert(const Xtc& tc, const void* payload) { return false; }

  InDatagramIterator* iterator(Pool* pool) const 
  { return new(pool) CDatagramIterator(_dg); }

  int  send   (ToNetEb&, const Ins&) { return -1; }
  int  send   (ToEb&) { return -1; }

private:
  Datagram& _dg;
};
    
class MyXtcMonitorClient : public XtcMonitorClient {
public:
  MyXtcMonitorClient(Appliance* app) :
    _pool(sizeof(MyDatagram),1) 
  {
    app->connect(&_inlet);
  }
  virtual void processDgram(Dgram* dg) {
    _inlet.post(new(&_pool) MyDatagram(*dg));
  };
private:
  MyInlet _inlet;
  GenericPool _pool;
};

void usage(char* progname) {
  fprintf(stderr,"Usage: %s [-t <partitionTag>] [-h]\n", progname);
}

int main(int argc, char** argv) {

  MonServerManager* manager = new MonServerManager(MonPort::Mon);
  CamDisplay* camdisp = new CamDisplay(*manager);

  AcqDisplay* acqdisp = new AcqDisplay(*manager);

  Appliance* apps = camdisp;
  acqdisp->connect(apps);

  int c=0;
  char partitionTag[128] = "";
  MyXtcMonitorClient myClient(apps);

  while ((c = getopt(argc, argv, "?ht:")) != -1) {
    switch (c) {
    case '?':
    case 'h':
      usage(argv[0]);
      exit(0);
    case 't':
      strcpy(partitionTag, optarg);
      // the run method will only return if it encounters an error
      fprintf(stderr, "myClient returned: %d\n", myClient.run(partitionTag));
      break;
    default:
      usage(argv[0]);
    }
  }
  if (c<1) usage(argv[0]);

  manager->dontserve();
  delete camdisp;
  delete acqdisp;
  delete manager;

  return 1;
}
