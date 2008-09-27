#ifndef PDS_EVENTTEST_HH
#define PDS_EVENTTEST_HH

#include "pds/management/EventCallback.hh"

namespace Pds {

class Task;
class EventOptions;
class CollectionManager;
class ErrLog;
class Arp;

class EventTest: public EventCallback {
public:
  EventTest(Task* task,
	    EventOptions& options,
	    Arp* arp);
  virtual ~EventTest();

  void attach(CollectionManager*);

private:
  // Implements EventCallback
  virtual void attached(SetOfStreams& streams);
  virtual void failed(Reason reason);
  virtual void dissolved(const Node& who);

private:
  Task* _task;
  EventOptions& _options;
  CollectionManager* _event;
};

}

#endif
