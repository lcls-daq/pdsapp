#include "pds/management/SourceLevel.hh"
#include "pds/service/Task.hh"

#include <stdlib.h>
#include <getopt.h>
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
  struct hostent* host = gethostbyname(argv[1]);
  if (host) {
    interface = htonl(*(in_addr_t*)host->h_addr_list[0]);
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
