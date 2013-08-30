#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "pds/service/Semaphore.hh"
#include "pds/service/Task.hh"
#include "pds/service/Timer.hh"
#include "pds/mon/MonConsumerClient.hh"
#include "pds/management/VmonClientManager.hh"
#include "pds/vmon/VmonRecorder.hh"

#include "pds/mon/MonPort.hh"

void printHelp(const char* program);

namespace Pds {
  class MyDriver : public MonConsumerClient,
		   public VmonClientManager,
		   public Timer {
  public:
    MyDriver(unsigned char platform,
	     const char*   partition,
             const char*   path) :
      VmonClientManager(platform, partition, *this),
      _recorder        (new VmonRecorder(path)),
      _task            (new Task(TaskObject("VmonTmr")))
    {
      VmonClientManager::start();
      if (!CollectionManager::connect()) {
	printf("platform %x unavailable\n",platform);
	exit(-1);
      }
      _recorder->enable();
      Timer::start();
    }
    ~MyDriver() { _recorder->disable(); }

    Task*    task() { return _task; }
    unsigned duration() const { return 1000; }
    void     expired() {
      _recorder->flush();
      request_payload();
    }
    unsigned repetitive() const { return 1; }
  private:
    // Implements MonConsumerClient
    void process(MonClient& client, MonConsumerClient::Type type, int result=0) {
      if (type==MonConsumerClient::Description)  _recorder->description(client);
      if (type==MonConsumerClient::Payload    )  _recorder->payload    (client);
    }
    void add(Pds::MonClient&) {}
  private:
    VmonRecorder* _recorder;
    Task*         _task;
  };
};

using namespace Pds;

static MyDriver* driver;

void sigintHandler(int iSignal)
{
  printf("vmonrecorder stopped by signal %d\n",iSignal);
  delete driver;
  exit(0);
}

int main(int argc, char **argv) 
{
  const unsigned NO_PLATFORM = (unsigned)-1;
  unsigned platform = NO_PLATFORM;
  const char* partition = 0;
  const char* path = ".";

  int c;
  while ((c = getopt(argc, argv, "p:P:o:")) != -1) {
    switch (c) {
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'P':
      partition = optarg;
      break;
    case 'o':
      path = optarg;
      break;
    default:
      printHelp(argv[0]);
      return 0;
    }
  }

  if (partition==0 || platform==NO_PLATFORM) {
    printHelp(argv[0]);
    return -1;
  }

  driver = new MyDriver(platform,
			partition,
                        path);

  // Unix signal support
  struct sigaction int_action;

  int_action.sa_handler = sigintHandler;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;

  if (sigaction(SIGINT, &int_action, 0) > 0)
    printf("Couldn't set up SIGINT handler\n");
  if (sigaction(SIGKILL, &int_action, 0) > 0)
    printf("Couldn't set up SIGKILL handler\n");
  if (sigaction(SIGSEGV, &int_action, 0) > 0)
    printf("Couldn't set up SIGSEGV handler\n");
  if (sigaction(SIGABRT, &int_action, 0) > 0)
    printf("Couldn't set up SIGABRT handler\n");
  if (sigaction(SIGTERM, &int_action, 0) > 0)
    printf("Couldn't set up SIGTERM handler\n");

  Semaphore sem(Semaphore::EMPTY);
  sem.take();

  sigintHandler(0);
  return 0;
}


void printHelp(const char* program)
{
  printf("usage: %s [-p <platform>] [-P <partition name>]\n", program);
}
