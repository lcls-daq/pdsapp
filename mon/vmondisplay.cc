#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pds/service/Task.hh"
#include "VmonMain.hh"

#include "pds/mon/MonPort.hh"

void printHelp(const char* program);

using namespace Pds;

int main(int argc, char **argv) 
{
  unsigned platform = -1UL;
  const char* partition = 0;

  int c;
  while ((c = getopt(argc, argv, "p:P:")) != -1) {
    switch (c) {
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'P':
      partition = optarg;
      break;
    default:
      printHelp(argv[0]);
      return 0;
    }
  }

  if (partition==0 || platform >0xff) {
    printHelp(argv[0]);
    return -1;
  }

  Task* workTask = new Task(TaskObject("VmonMain"));
  VmonMain dspl(workTask, platform, partition);
}


void printHelp(const char* program)
{
  printf("usage: %s [-p <platform>] [-P <partition name>]\n", program);
}
