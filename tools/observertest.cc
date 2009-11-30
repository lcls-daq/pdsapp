#include "pds/service/Task.hh"
#include "EventOptions.hh"
#include "EventTest.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
using namespace Pds;

void usage(char* progname) {
  printf("Usage: %s -P <partition> -i <node mask>\n", progname);
}

int main(int argc, char** argv) {

  const char* partition = 0;
  unsigned nodes =  0;
  char* endPtr;
  char** aarg  = argv;
  char** aarge = aarg+argc;
  while(++aarg < aarge) {
    if (strcmp(*aarg,"-i")==0) {
      nodes = strtoul(*++aarg, &endPtr, 0);
      break;
    }
    else if (strcmp(*aarg,"-P")==0)
      partition = *++aarg;
  }
  printf("partition %s  nodes %x\n",partition,nodes);

  EventOptions options(aarge-aarg, aarg);
  if (!options.validate(argv[0]))
    return 0;

  if (nodes == 0) {
    fprintf(stderr, "Missing parameters!\n");
    usage(argv[0]);
    return 1;
  }

  Task* task = new Task(Task::MakeThisATask);
  EventTest* test = new EventTest(task, options, 0);

  ObserverLevel* event = new ObserverLevel(options.platform,
					   partition,
					   nodes,
					   *test);

  if (event->attach())
    task->mainLoop();
  else
    printf("Observer failed to attach to platform\n");

  event->detach();

  delete event;
  return 0;
}

