#include "pds/management/SourceLevel.hh"
#include "pds/service/Task.hh"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace Pds;

void usage(const char* p)
{
  printf("Usage: %s <interface> [-h]\n", p);
}

int main(int argc, char** argv)
{
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

  if (parseErr || (argc != 2)) {
    usage(argv[0]);
    exit(1);
  }

  unsigned interface = 0;
  in_addr inp;
  if (inet_aton(argv[1], &inp)) {
    interface = ntohl(inp.s_addr);
  }

  if (!interface) {
    printf("Invalid <interface> argument %s\n", argv[1]);
    exit(1);
  }

  Task* task = new Task(Task::MakeThisATask);

  SourceLevel source;
  source.start();
  if (source.connect(interface)) 
    task->mainLoop();

  return 0;
}
