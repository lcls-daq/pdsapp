#include <stdio.h>

#include "EventTest.hh"
#include "EventOptions.hh"
#include "DgSummary.hh"

#include "pds/management/PartitionMember.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/client/Decoder.hh"
#include "CountAction.hh"
//#include "StatsApp.hh"
#include "StatsTree.hh"
#include "Recorder.hh"
#include "pds/service/Task.hh"

using namespace Pds;


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

bool EventTest::attach(PartitionMember* event)
{
  _event = event;
  return event->attach();
}

void EventTest::attached(SetOfStreams& streams)
{
  printf("EventTest connected to platform 0x%x\n", _event->header().platform());
  
  Stream* frmk = streams.stream(StreamParams::FrameWork);

  //  Send event summaries to ControlLevel
  {
    //    frmk->outlet()->sink(TransitionId::L1Accept);
    frmk->outlet()->sink(TransitionId::Unknown);

    (new DgSummary)->connect(frmk->inlet());
  }
  
  //  Stream* occr = streams.stream(StreamParams::Occurrence);
  //  occr->outlet()->sink(OccurrenceId::Vmon);

  if (_options.outfile) (new Recorder(_options.outfile, _options.sliceID, _options.chunkSize))->connect(frmk->inlet());
  
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
      //      const Node& node = _event->header();
      //      (new StatsApp(node.procInfo()))->connect(frmk->inlet());
      (new StatsTree(static_cast<EbBase*>(streams.wire())))->connect(frmk->inlet());
      break;
    }
  }

}

void EventTest::detach()
{
  _event->detach();
}

void EventTest::failed(Reason reason) 
{
  static const char* reasonname[] = { "platform unavailable", 
				      "crates unavailable", 
				      "fcpm unavailable" };
  printf("EventTest: unable to allocate crates on platform 0x%x : %s\n", 
	 _event->header().platform(), reasonname[reason]);
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
  
  printf("EventTest: platform 0x%x dissolved by user %s, pid %d, on node %s", 
          who.platform(), username, who.pid(), ipname);

  delete this;
}
