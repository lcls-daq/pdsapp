#include <stdio.h>

#include "EventTest.hh"
#include "EventOptions.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/utility/SetOfStreams.hh"
//#include "EventDisplay.hh"
#include "pds/client/Decoder.hh"
//#include "DebugAction.hh"
//#include "CountAction.hh"
//#include "NoAction.hh"
//#include "Fsm.hh"
#include "pds/service/Task.hh"

using namespace Pds;

namespace Pds {

class CountAction: public Appliance {
public:
  CountAction() : _counter(0), _transitions(0) {}
  ~CountAction()
  {
    printf("CountAction: %.5d transitions\n"
	   "             %.5d events\n",
	   _transitions, _counter);
  }
  virtual Transition* transitions(Transition* in)
  {
    ++_transitions;
    printf ("transition %d\n", _transitions);
    return in;
  }

  virtual InDatagram* events(InDatagram* in) 
  {
    const Datagram& dg = in->datagram();
    if (_seq >= dg) printf("%x/%x was out of order. Last was %x/%x. Size %i\n",
			   dg.high(), dg.low(), _seq.high(),
			   _seq.low(), dg.xtc.sizeofPayload());
    _seq = dg;
    if (!(++_counter%1000)){
      printf ("nevents      = %d\n", _counter);
      printf ("current size = %d\n\n", dg.xtc.sizeofPayload());
    }
    return in;
  }
  virtual InDatagram* occurrences(InDatagram* dg) { return dg; }
private:
  unsigned _counter;
  unsigned _transitions;
  Sequence _seq;
};
}

EventTest::EventTest(Task* task,
		     EventOptions& options,
		     Arp* arp) :
  _task(task),
  _options(options)
{
}

EventTest::~EventTest()
{
  delete _event;
  _task->destroy();
}

void EventTest::attach(CollectionManager* event)
{
  _event = event;
  _event->dotimeout(250);
  _event->connect();
}

void EventTest::attached(SetOfStreams& streams) 
{
  printf("EventTest connected to partition 0x%x\n", _event->header().partition());

  Stream* frmk = streams.stream(StreamParams::FrameWork);
  if (_event->header().level()==Level::Recorder)
    frmk->outlet()->sink(EventId::L1Accept);

  //  Stream* occr = streams.stream(StreamParams::Occurrence);
  //  occr->outlet()->sink(OccurrenceId::Vmon);

  switch (_options.mode) {
  case EventOptions::Counter:
    {
      (new CountAction)->connect(frmk->inlet());
      break;
    }
  case EventOptions::Decoder:
    {
      (new Decoder(Level::Event))->connect(frmk->inlet());
      //      (new Decoder)->connect(occr->inlet());
      break;
    }
  case EventOptions::Display:
    {
      break;
    }
  }
}

void EventTest::failed(Reason reason) 
{
  static const char* reasonname[] = { "platform unavailable", 
				      "crates unavailable", 
				      "fcpm unavailable" };
  printf("EventTest: unable to allocate crates on partition 0x%x : %s\n", 
	 _event->header().partition(), reasonname[reason]);
  delete this;
}

void EventTest::dissolved(const Node& who) 
{
  const unsigned userlen = 12;
  char username[userlen];
  Node::user_name(who.uid(),username,userlen);

  const unsigned iplen = 64;
  char ipname[iplen];
  Node::ip_name(who.ip(),ipname, iplen);
  
  printf("EventTest: partition 0x%x dissolved by user %s, pid %d, on node %s", 
          who.partition(), username, who.pid(), ipname);

  delete this;
}
