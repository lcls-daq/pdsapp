#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"

#include <string>
#include <unistd.h>

using namespace Pds;

namespace Pds {
class MyRoutine : public Routine {
public:
  MyRoutine(const char* s) : _s(s) {}
public:
  void routine() { printf("Executing %s\n",_s.c_str()); }
private:
  std::string _s;
};
};

int main(int argc, char* argv[])
{
  Routine* r;

  if (argc>1)
  { Queue<Routine>* _jobs = new Queue<Routine>;
    Semaphore* _pending = new Semaphore(Semaphore::EMPTY);
    r = new MyRoutine("First");
    if( _jobs->insert(r) == _jobs->empty()) {
      _pending->give();
    }
    r = new MyRoutine("Second");
    if( _jobs->insert(r) == _jobs->empty()) {
      _pending->give();
    }
   Routine *aJob;
   while( (aJob=_jobs->remove()) != _jobs->empty() ) {
      printf("Executing routine %p [%p]\n",aJob, _jobs->empty());
      aJob->routine();
    }
  }

  Task* task = new Task(TaskObject("ttest"));
  usleep(100);
  r = new MyRoutine("First");
  printf("Queuing %p\n",r);
  task->call(r);
  usleep(100);
  r = new MyRoutine("Second");
  printf("Queuing %p\n",r);
  task->call(r);
  usleep(100);
  return 0;
}
