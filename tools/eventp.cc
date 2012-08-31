#include "ParasiticRecorder.hh"
#include "EventOptions.hh"
#include "pds/service/Task.hh"
#include "pds/management/ObserverLevel.hh"

using namespace Pds;

void usage(const char* p)
{
  printf("Usage: %s -p <platform#> -P <partition name> -i <node mask> \n", p);
  printf("\t[-f <output path>] [-s <sliceID>] [-c <chunkSize>] [-t <file lifetime sec>]\n");
  printf("\t[-L <offlinerc> -E <experiment name>]\n");
}

int main(int argc, char** argv) 
{
  const char* partition = 0;
  const char* offlinerc = 0;
  unsigned nodes = 0;
  unsigned lifetime = 36000;
  EventOptions options;

  int c;
  while ((c = getopt(argc, argv, "f:p:s:c:edP:L:E:i:t:b:")) != -1) {
    char* endPtr;
    switch(c) {
    case 'f':  options.outfile = optarg; break;
    case 'p':  options.platform = strtoul(optarg, &endPtr, 0); break;
    case 's':  options.sliceID  = strtoul(optarg, &endPtr, 0); break;
    case 'c':  options.chunkSize= strtoull(optarg, &endPtr, 0); break;
    case 'P':  partition = optarg; break;
    case 'L':  offlinerc = optarg; break;
    case 'E':  options.expname = optarg; break;
    case 'i':  nodes = strtoul(optarg, &endPtr, 0); break;
    case 't':  lifetime = strtoul(optarg, &endPtr, 0); break;
    case 'e':  options.mode = EventOptions::Decoder; break;
    case 'd':  options.mode = EventOptions::Display; break;
    case 'b':  options.buffersize = strtoul(optarg, &endPtr,0); break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  int err = 0;
  if (!options.validate(argv[0])) {
    err++;
  }
  if (partition == 0) {
    printf("Partition (-P) not specified.\n");
    err++;
  }
  if (nodes == 0) {
    printf("Event nodes (-i) not specified.  Only transitions will be recorded.\n");
  }
  if (options.outfile == 0) {
    printf("Output file path (-f) not specified.  Nothing will be recorded.\n");
  }
  if (offlinerc == 0) {
    printf("Offlinerc path (-L) not specified.  Database notifications will not be sent.\n");
  } else if (options.expname == 0) {
    printf("Experiment name (-E) not specified, required when -L (offlinerc path) is specified.\n");
    err++;
  }

  if (err) 
    exit(1);

  Task* task = new Task(Task::MakeThisATask);
  ParasiticRecorder* test = new ParasiticRecorder(task, options, lifetime, partition, offlinerc);
  ObserverLevel* event = new ObserverLevel(options.platform,
					   partition,
					   nodes,
					   *test);
  if (event->attach())
    task->mainLoop();

  event->detach();

  return 0;
}

