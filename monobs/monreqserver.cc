#include "pdsdata/app/XtcMonitorServer.hh"

#include <errno.h>

namespace Pds {
  class MyXtcMonitorServer : public XtcMonitorServer {
  public:
    MyXtcMonitorServer(const char* tag,
		       unsigned sizeofBuffers, 
		       unsigned numberofEvBuffers, 
		       unsigned numberofEvQueues) : 
      XtcMonitorServer(tag,
		       sizeofBuffers,
		       numberofEvBuffers,
		       numberofEvQueues) 
    {
      _init();
    }
    ~MyXtcMonitorServer() {}
  public:
    void run() {
      while(1)
	sleep(1);
    }
  private:
    void _copyDatagram(Dgram* dg, char*) {}
    void _deleteDatagram(Dgram* dg) {}
    void _requestDatagram() {}
  };
};

using namespace Pds;

void usage(char* progname) {
  printf("Usage: %s -p <platform> -P <partition> -i <node mask> -n <numb shm buffers> -s <shm buffer size> [-q <# event queues>] [-t <tag name>] [-d] [-c] [-g <max groups>] [-h]\n", progname);
}

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = unsigned(-1UL);
  unsigned platform=NO_PLATFORM;
  const char* partition = 0;
  const char* tag = 0;
  int numberOfBuffers = 0;
  unsigned sizeOfBuffers = 0;
  unsigned nevqueues = 1;
  unsigned node =  0xffff;
  unsigned nodes = 6;
  bool ldist = false;

  int c;
  while ((c = getopt(argc, argv, "p:i:g:n:P:s:q:t:dch")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = NO_PLATFORM;
      break;
    case 'i':
      node = strtoul(optarg, &endPtr, 0);
      break;
    case 'g':
      nodes = strtoul(optarg, &endPtr, 0);
      break;
    case 'n':
      sscanf(optarg, "%d", &numberOfBuffers);
      break;
    case 'P':
      partition = optarg;
      break;
    case 't':
      tag = optarg;
      break;
    case 'q':
      nevqueues = strtoul(optarg, NULL, 0);
      break;
    case 's':
      sizeOfBuffers = (unsigned) strtoul(optarg, NULL, 0);
      break;
    case 'd':
      ldist = true;
      break;
    case 'h':
      // help
      usage(argv[0]);
      return 0;
      break;
    default:
      printf("Unrecogized parameter\n");
      usage(argv[0]);
      break;
    }
  }

  if (!numberOfBuffers || !sizeOfBuffers || platform == NO_PLATFORM || !partition || node == 0xffff) {
    fprintf(stderr, "Missing parameters!\n");
    usage(argv[0]);
    return 1;
  }

  if (numberOfBuffers<8) numberOfBuffers=8;

  if (!tag) tag=partition;

  printf("\nPartition Tag:%s\n", tag);

  MyXtcMonitorServer* apps = new MyXtcMonitorServer(tag, 
						    sizeOfBuffers, 
						    numberOfBuffers, 
						    nevqueues);
  apps->distribute(ldist);

  apps->run();
  
  return 0;
}

