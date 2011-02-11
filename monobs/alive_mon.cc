#include "pds/service/Task.hh"
#include "pds/service/Timer.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/app/XtcMonitorClient.hh"

#include <stdlib.h>
#include <unistd.h>

using namespace Pds;

class MyAlarm : public Timer {
public:
  MyAlarm(unsigned duration,
          unsigned min_events,
          unsigned max_damage,
          const char* system_call) : 
    _duration   (duration),
    _events     (0),
    _min_events (min_events),
    _damage     (0),
    _max_damage (max_damage),
    _system_call(system_call),
    _task       (new Task(TaskObject("AlmTmr")))
  {}
  ~MyAlarm() { _task->destroy(); }
public:
  void  expired      () { 
    if (_events < _min_events ||
        _damage > _max_damage) {
      char buff[256];
      time_t now = time(0);
      sprintf(buff,"echo \"alive_mon: %d evts, %d dmg, in %d sec: %s\" | %s",
              _events,_damage,_duration/1000,ctime(&now),_system_call);
      printf("%s\n",buff);
      system(buff);
    }
    _events = 0;
    _damage = 0;
  }
  Task* task         () { return _task; }
  unsigned duration  () const { return _duration; }
  unsigned repetitive() const { return 1; }
public:
  void event () { _events++; }
  void damage() { _damage++; }
private:
  unsigned   _duration;
  unsigned   _events, _min_events;
  unsigned   _damage, _max_damage;
  const char* _system_call;
  Task*      _task;
};

class MyMonitorClient : public Pds::XtcMonitorClient {
public:
  MyMonitorClient(MyAlarm& alarm) : _alarm(alarm) {}
public:
  int processDgram(Dgram* dg) {
    if (dg->seq.isEvent()) {
      _alarm.event();
      if (dg->xtc.damage.value())
        _alarm.damage();
    }
    return 0;
  }
private:
  MyAlarm& _alarm;
};


void usage(const char* p)
{
  printf("Usage: %s -p <partitionTag> -i <shm index> -t <check interval> -e <min evts> -d <max dmg> -s <system_call>\n",
	 p);
}

int main(int argc, char* argv[])
{
  unsigned index = 0;
  const char* partitionTag = 0;
  unsigned duration = 10000;
  unsigned events   = 1;
  unsigned damage   = 0;
  const char* system_call = "/bin/true";

  int c;
  while ((c = getopt(argc, argv, "p:i:t:e:d:s:h?")) != -1) {
    switch (c) {
    case 'p':
      partitionTag = optarg;
      break;
    case 'i':
      index = strtoul(optarg,NULL,0);
      break;
    case 't':
      duration = 1000*strtoul(optarg,NULL,0);
      break;
    case 'e':
      events = strtoul(optarg,NULL,0);
      break;
    case 'd':
      damage = strtoul(optarg,NULL,0);
      break;
    case 's':
      system_call = optarg;
      break;
    case '?':
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    }
  }
  
  MyAlarm alarm(duration,events,damage,system_call);
  alarm.start();
  MyMonitorClient client(alarm);
  fprintf(stderr, "client returned: %d\n", client.run(partitionTag, index, index));
}
