#include "EventTest.hh"
#include "EventOptions.hh"
#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/RecorderLevel.hh"

using namespace Pds;

int main(int argc, char** argv) {
  EventOptions options(argc, argv);
  if (!options.validate(argv[0]))
    return 0;

  Arp* arp=0;
  if (options.arpsuidprocess) {
    arp = new Arp(options.arpsuidprocess);
    if (arp->error()) {
      char message[128];
      sprintf(message, "failed to create Arp for `%s': %s", 
	      options.arpsuidprocess, strerror(arp->error()));
      printf("%s: %s\n", argv[0], message);
      delete arp;
      return 0;
    }
  }

  Task* task = new Task(Task::MakeThisATask);
  EventTest* test = new EventTest(task, options, arp);
  RecorderLevel* event = new RecorderLevel(options.platform,
					   *test,
					   arp);
  test->attach(event);
  task->mainLoop();
  if (arp) delete arp;
  return 0;
}

