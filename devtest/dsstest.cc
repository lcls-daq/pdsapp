#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Task.hh"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/file.h>
#include <list>

class SemQ : public Pds::Routine {
public:
  SemQ(Pds::Semaphore& sem) : _sem(sem) {}
public:
  void routine() { _sem.give(); }
private:
  Pds::Semaphore& _sem;
};

class Stats {
public:
  Stats(unsigned len) : _events(0), _times(new double[len]) {}
public:
  void fill(timespec& begin, timespec& end) 
  {
    double dt = double(end.tv_sec - begin.tv_sec) +
      1.e-9*(double(end.tv_nsec)-double(begin.tv_nsec));

    _times[_events++] = dt;
  }
  void dump(const char* fname) const
  {
    FILE* f = fopen(fname,"w");
    for(unsigned i=0; i<_events; i++)
      fprintf(f,"%g\n",_times[i]);
    fclose(f);
  }
private:
  unsigned _events;
  double* _times;
};

static Stats* _stats;

class RecordQ : public Pds::Routine {
public:
  RecordQ(char* event, unsigned eventsize,
          FILE* f,
          std::list<char*>& buffers) :
    _event(event), _eventsize(eventsize), _f(f), _buffers(buffers) {}
  ~RecordQ() {}
public:
  void routine()
  {
    timespec begin,end;
    clock_gettime(CLOCK_REALTIME,&begin);
    fwrite(_event, _eventsize, 1, _f);
    clock_gettime(CLOCK_REALTIME,&end);
    _stats->fill(begin,end);

    _buffers.push_back(_event);
  }
private:
  char*             _event;
  unsigned          _eventsize;
  FILE*             _f;
  std::list<char*>& _buffers;
};

static void usage(const char* p)
{
  printf("%s [options]\n"
         "[-f <fname>]       : data file name\n"
         "[-F <fname>]       : stats file name\n"
         "[-b <nbuffers>]    : number of buffers\n"
         "[-B <buffer size>] : buffer size (bytes)\n"
         "[-e <nevents>]     : number of events\n"
         "[-E <event size>]  : event size (bytes)\n",
         p);
}

int main(int argc, char** argv) {

  const char* fname   = "tst.dat";
  const char* sname   = "tst.log";
  unsigned buffersize = 20*1024*1024;
  unsigned nbuffers   = 16;
  unsigned eventsize  = 0x100000;
  unsigned nevents    = 1000;

  extern char* optarg;
  char c;
  while((c=getopt(argc,argv,"f:F:b:B:e:E:h"))!=-1) {
    switch(c) {
    case 'f': fname      = optarg; break;
    case 'F': sname      = optarg; break;
    case 'b': nbuffers   = strtoul(optarg,NULL,0); break;
    case 'B': buffersize = strtoul(optarg,NULL,0); break;
    case 'e': nevents    = strtoul(optarg,NULL,0); break;
    case 'E': eventsize  = strtoul(optarg,NULL,0); break;
    case 'h': default: usage(argv[0]); exit(1);
    }
  }

  _stats  = new Stats(nevents);

  FILE* f = fopen(fname,"wx");
  if (!f) {
    perror(fname);
    return -2;
  }

  int rc;
  struct flock flk;
  flk.l_type   = F_WRLCK;
  flk.l_whence = SEEK_SET;
  flk.l_start  = 0;
  flk.l_len    = 0;
  do { rc = fcntl(fileno(f), F_SETLKW, &flk); }
  while(rc<0 && errno==EINTR);
  if (rc<0) {
    perror(fname);
    return -3;
  }
  setvbuf(f, NULL, _IOFBF, 4*1024*1024);
    
  std::list<char*> buffers;
  for(unsigned i=0; i<nbuffers; i++)
    buffers.push_back(new char[buffersize]);

  char* event = new char[eventsize];
  for(unsigned i=0; i<eventsize; i++)
    event[i] = i&0xff;

  Pds::Task* task = new Pds::Task(Pds::TaskObject("RecTask"));

  for(unsigned i=0; i<nevents; i++) {

    if (i%(nevents/10)==0)
      printf("%d %% \n",i*100/nevents);

    while(buffers.empty()) 
      usleep(1000);
    char* eventb = buffers.front();
    buffers.pop_front();
    memcpy(eventb,event,eventsize);
    task->call(new RecordQ(eventb,eventsize,f,buffers));
  }

  Pds::Semaphore sem(Pds::Semaphore::EMPTY);
  task->call(new SemQ(sem));
  sem.take();

  _stats->dump(sname);

  return 1;
}
