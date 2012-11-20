#include "pds/management/SourceLevel.hh"
#include "pds/service/Task.hh"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace Pds;

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("usage: %s <interface>\n", argv[0]);
    return 0;
  }

  unsigned interface = 0;
  in_addr inp;
  if (inet_aton(argv[1], &inp)) {
    interface = ntohl(inp.s_addr);
  }

  if (!interface) {
    printf("Invalid <interface> argument %s\n", argv[1]);
    return 0;    
  }

  Task* task = new Task(Task::MakeThisATask);

  SourceLevel source;
  source.start();
  if (source.connect(interface)) 
    task->mainLoop();

  return 0;
}
