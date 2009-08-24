#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/service/GenericPool.hh"

#include "pds/mon/MonServerManager.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#ifdef _POSIX_MESSAGE_PASSING
#include <mqueue.h>
#endif
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

#define PERMS (S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH)
#define OFLAGS (O_CREAT|O_RDWR)

using namespace Pds;


class Msg {
  public:
    Msg() {}
    Msg(int bufferIndex) {_bufferIndex = bufferIndex;}
    ~Msg() {}; 
    Msg* bufferIndex(int b) {_bufferIndex=b; return this;}
    int bufferIndex() {return _bufferIndex;}
    void numberOfBuffers(int n) {_numberOfBuffers = n;} 
    void sizeOfBuffers(int s) {_sizeOfBuffers = s;} 
  private:
    int _bufferIndex;
    int _numberOfBuffers;
    unsigned _sizeOfBuffers;
};


class XtcMonServer : public Appliance {
public:
  XtcMonServer(unsigned s, int n) : 
    _sizeOfBuffers(s),
    _numberOfBuffers(n),
    _bufferCount(0),
    _linked(true),
    _priority(0)
  { _myMsg.numberOfBuffers(n);
    _myMsg.sizeOfBuffers(s);
  }
  ~XtcMonServer() 
  { if (_linked) 
    { printf("Not Unlinking ... \n");
//      if (mq_unlink(_toMonQname) == (mqd_t)-1) perror("mq_unlink To Monitor");
//      if (mq_unlink(_fromMonQname) == (mqd_t)-1) perror("mq_unlink From Monitor");
//      shm_unlink(_shmName);
//      printf("Finished.\n");
    }
  }
public:
  Transition* transitions(Transition* tr) { return tr; }
  InDatagram* occurrences(InDatagram* dg) { return dg; }
  
  InDatagram* events     (InDatagram* dg) 
  { mq_getattr(_myInputQueue, &_mymq_attr);
    Datagram& dgrm = dg->datagram();
    // reserve the last four buffers for transitions
    if ((_mymq_attr.mq_curmsgs > 4) || ((dgrm.seq.service() != TransitionId::L1Accept) && _mymq_attr.mq_curmsgs))
    {
      if (mq_receive(_myInputQueue, (char*)&_myMsg, sizeof(_myMsg), &_priority) < 0) perror("mq_receive");
      _bufferP = _myShm + (_sizeOfBuffers * _myMsg.bufferIndex());
      //  write the datagram
      memcpy((char*)_bufferP, &dgrm, sizeof(Datagram));
      unsigned offset = sizeof(Datagram);
      //  write the payload
      InDatagramIterator& iter = *dg->iterator(_pool);
      struct iovec iov;
      int remaining = dgrm.xtc.sizeofPayload();
      while(remaining) {
        int isize = iter.read(&iov,1,remaining);
	/*
        printf("Iterator found %x bytes at %p dgrm at %p next %p\n",
               iov.iov_len,iov.iov_base,&dgrm,
               dgrm.xtc.next());
	*/
	memcpy(_bufferP+offset, iov.iov_base, iov.iov_len);
	offset += iov.iov_len;
        remaining -= isize;
      }
      delete &iter;
      if (mq_send(_myOutputQueue, (const char*)&_myMsg, sizeof(_myMsg), 0)) perror("mq_send");
      _bufferCount += 1;
    }
     return dg;
  }

  int init(char *p) 
  { 
    strcpy(_partitionTag, "");
    strcpy(_shmName, "/PdsMonitorSharedMemory_");
    strcpy(_toMonQname, "/PdsToMonitorMsgQueue_");
    strcpy(_fromMonQname, "/PdsFromMonitorMsgQueue_");
    _pageSize = (unsigned)sysconf(_SC_PAGESIZE);
  
    strcat(_toMonQname, p);
    strcat(_fromMonQname, p);
    strcat(_shmName, p);

    int ret = 0;
    _sizeOfShm = _numberOfBuffers * _sizeOfBuffers;
    unsigned remainder = _sizeOfShm%_pageSize;
    if (remainder) _sizeOfShm += _pageSize - remainder;

    _mymq_attr.mq_maxmsg = _numberOfBuffers+4;
    _mymq_attr.mq_msgsize = (long int)sizeof(Msg);
    _mymq_attr.mq_flags = 0L;

    umask(1);  // try to enable others to open these devices.

//    if (!shm_unlink(_shmName)) perror("shm_unlink found a remnant of previous lives");
    int shm = shm_open(_shmName, OFLAGS, PERMS);
    if (shm < 0) {ret++; perror("shm_open");}

    if ((ftruncate(shm, _sizeOfShm))<0) {ret++; perror("ftruncate");}

    _myShm = (char*)mmap(NULL, _sizeOfShm, PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
    if (_myShm == MAP_FAILED) {ret++; perror("mmap");}

//    if (mq_unlink(_toMonQname) != (mqd_t)-1) perror("mq_unlink To Monitor found a remnant of previous lives");
//    if (mq_unlink(_fromMonQname) != (mqd_t)-1) perror("mq_unlink From Monitor found a remnant of previous lives");
    _myOutputQueue = mq_open(_toMonQname, O_CREAT|O_RDWR, PERMS, &_mymq_attr);
    if (_myOutputQueue == (mqd_t)-1) {ret++; perror("mq_open output");}
    _myInputQueue = mq_open(_fromMonQname, O_CREAT|O_RDWR, PERMS, &_mymq_attr);
    if (_myInputQueue == (mqd_t)-1) {ret++; perror("mq_open input");}

    // flush the queues just to be sure they are empty.
    Msg m;
    do {
      mq_getattr(_myInputQueue, &_mymq_attr);
      if (_mymq_attr.mq_curmsgs)
           mq_receive(_myInputQueue, (char*)&m, sizeof(m), &_priority);
     } while (_mymq_attr.mq_curmsgs);

    do {
      mq_getattr(_myOutputQueue, &_mymq_attr);
      if (_mymq_attr.mq_curmsgs)
            mq_receive(_myOutputQueue, (char*)&m, sizeof(m), &_priority);
    } while (_mymq_attr.mq_curmsgs);


    // prestuff the input queue which doubles as the free list
    for (int i=0; i<_numberOfBuffers; i++) {
      if (mq_send(_myInputQueue, (const char *)_myMsg.bufferIndex(i), sizeof(Msg), 0)) 
      { ret++; perror("mq_send inQueueStuffing");
      }
    }
    _pool = new GenericPool(sizeof(ZcpDatagramIterator),1);
    return ret;
  }

private:
  Pool* _pool;
  char _partitionTag[80];
  unsigned _sizeOfBuffers;
  int _numberOfBuffers;
  int _bufferCount;
  bool _linked;
  unsigned _sizeOfShm;
  char *_bufferP;   //  pointer to the shared memory area being used
  char *_myShm; // the pointer to start of shared memory
  mqd_t _myOutputQueue;
  mqd_t _myInputQueue;
  unsigned _priority;
  char _shmName[128];
  char _toMonQname[128];
  char _fromMonQname[128];
  unsigned _pageSize;
  struct mq_attr _mymq_attr;
  Msg _myMsg;
};

class MyCallback : public EventCallback {
public:
  MyCallback(Task* task, Appliance* app) :
    _task(task), 
    _appliances(app)
  {
  }
  ~MyCallback() {}

  void attached (SetOfStreams& streams) 
  {
    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _appliances->connect(frmk->inlet());
  }
  void failed   (Reason reason)   { _task->destroy(); delete this; }
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  Appliance*  _appliances;
};

void usage(char* progname) {
  printf("Usage: %s -p <platform> -P <partition> -i <monitor node> -n <numb shm buffers> -s <shm buffer size>\n", progname);
}

Appliance* apps;

void sigfunc(int sig_no) {
   delete apps;
   exit(EXIT_SUCCESS);
}

void exit_failure() {
  delete apps;
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {

  unsigned platform=-1UL;
  const char* partition = 0;
  int numberOfBuffers = 0;
  unsigned sizeOfBuffers = 0;
  unsigned node =  0xffff0;
  char partitionTag[80] = "";
  (void) signal(SIGINT, sigfunc);
  if (prctl(PR_SET_PDEATHSIG, SIGINT) < 0) printf("Changing death signal failed!\n");
  int c;
  while ((c = getopt(argc, argv, "p:i:n:P:s:")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = -1UL;
      break;
    case 'i':
      node = strtoul(optarg, &endPtr, 0);
      break;
    case 'n':
      sscanf(optarg, "%d", &numberOfBuffers);
      break;
    case 'P':
      partition = optarg;
      break;
    case 's':
      sizeOfBuffers = (unsigned) strtoul(optarg, NULL, 0);
      break;
    default:
      break;
    }
  }

  if (!numberOfBuffers || !sizeOfBuffers || platform == -1UL || !partition || node == 0xffff) {
    fprintf(stderr, "Missing parameters!\n");
    usage(argv[0]);
    return 1;
  }

  if (numberOfBuffers<8) numberOfBuffers=8;

  sprintf(partitionTag, "%d_", platform);
  char temp[100];
  sprintf(temp, "%d_", node);
  strcat(partitionTag, temp);
  strcat(partitionTag, partition);
  printf("\nPartition Tag:%s\n", partitionTag);


  apps = new XtcMonServer(sizeOfBuffers, numberOfBuffers);
  if (((XtcMonServer*)apps)->init(partitionTag))
  { fprintf(stderr, "Initializing XTC monitor server encountered an error!\n");
    exit_failure();
  };
  
  Task* task = new Task(Task::MakeThisATask);
  MyCallback* display = new MyCallback(task, 
				       apps);

  ObserverLevel* event = new ObserverLevel(platform,
					   partition,
					   node,
					   *display);

  if (event->attach())
    task->mainLoop();
  else
    printf("Observer failed to attach to platform\n");

  event->detach();

  delete apps;
  delete event;
  delete display;
  return 0;
}

