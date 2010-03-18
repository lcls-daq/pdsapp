#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/service/GenericPool.hh"
#include "pdsapp/tools/PnccdShuffle.hh"

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

#include "pds/service/Semaphore.hh"
#include "pds/service/Task.hh"
#include <poll.h>
#include <queue>
#include <stack>

using std::queue;
using std::stack;

#define PERMS (S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH)
//#define PERMS (S_IRUSR|S_IWUSR)
#define OFLAGS (O_CREAT|O_RDWR)

using namespace Pds;


class Msg {
  public:
    Msg() {}
    Msg(int bufferIndex) {_bufferIndex = bufferIndex;}
    ~Msg() {}; 
    int bufferIndex() const {return _bufferIndex;}
    int numberOfBuffers() const { return _numberOfBuffers; }
    int sizeOfBuffers() const { return _sizeOfBuffers; }
    Msg* bufferIndex(int b) {_bufferIndex=b; return this;}
    void numberOfBuffers(int n) {_numberOfBuffers = n;} 
    void sizeOfBuffers(int s) {_sizeOfBuffers = s;} 
  private:
    int _bufferIndex;
    int _numberOfBuffers;
    unsigned _sizeOfBuffers;
};

class ShMsg {
public:
  ShMsg() {}
  ShMsg(const Msg&  m,
	InDatagram* dg) : _m(m), _dg(dg) {}
  ~ShMsg() {}
public:
  const Msg&  msg() const { return _m; }
  InDatagram* dg () const { return _dg; }
private:
  Msg _m;
  InDatagram* _dg;
};

class XtcMonServer : public Appliance,
		     public Routine {
public:
  enum { numberofTrBuffers=8 };
public:
  XtcMonServer(unsigned sizeofBuffers, int numberofEvBuffers, unsigned numberofClients) : 
    _sizeOfBuffers(sizeofBuffers),
    _numberOfEvBuffers(numberofEvBuffers),
    _numberOfClients  (numberofClients),
    _priority(0),
    _task(0),
    _sem(Semaphore::FULL)
  {
    _myMsg.numberOfBuffers(numberofEvBuffers+numberofTrBuffers);
    _myMsg.sizeOfBuffers(sizeofBuffers);

    _tmo.tv_sec  = 0;
    _tmo.tv_nsec = 0;
  }
  ~XtcMonServer() 
  { 
    printf("Not Unlinking ... \n");
    _task->destroy();
  }

public:
  Transition* transitions(Transition* tr) 
  {
    if (tr->id() == TransitionId::Unmap) 
      _pop_transition();
    return tr;
  }

  InDatagram* occurrences(InDatagram* dg) { return dg; }
  
  InDatagram* events     (InDatagram* dg) 
  {
    Datagram& dgrm = dg->datagram();
    if (dgrm.seq.service() == TransitionId::L1Accept) {
      mq_getattr(_myInputEvQueue, &_mymq_attr);
      if (_mymq_attr.mq_curmsgs) {
	if (mq_receive(_myInputEvQueue, (char*)&_myMsg, sizeof(_myMsg), &_priority) < 0) 
	  perror("mq_receive");
	
	ShMsg m(_myMsg, dg);
	if (mq_timedsend(_shuffleQueue, (const char*)&m, sizeof(m), 0, &_tmo)) {
	  printf("ShuffleQ timedout\n");
	  return dg;
	}
	
	return (InDatagram*)Appliance::DontDelete;
      }
    }
    else {

      if (_freeTr.empty()) {
	printf("No buffers available for transition!\n");
	abort();
      }

      int ibuffer = _freeTr.front(); _freeTr.pop();

      _myMsg.bufferIndex(ibuffer);
      _copyDatagram(dg, ibuffer);

      _sem.take();
      if (unsigned(dgrm.seq.service())%2) {
	_pop_transition();
	_freeTr.push(ibuffer);
      }
      else 
	_push_transition(ibuffer);
      _sem.give();
      
      for(unsigned i=0; i<_numberOfClients; i++) {
// 	printf("Sending tr %s to mq %d\nmsg %x/%x/%x\n",
// 	       TransitionId::name(dgrm.seq.service()), 
// 	       _myOutputTrQueue[i],
// 	       _myMsg.bufferIndex(),
// 	       _myMsg.numberOfBuffers(),
// 	       _myMsg.sizeOfBuffers());
	if (mq_timedsend(_myOutputTrQueue[i], (const char*)&_myMsg, sizeof(_myMsg), 0, &_tmo))
	  ;  // best effort
      }

      _flushQueue(_myOutputEvQueue);
    }

    return dg;
  }

  void routine()
  {
    while(1) {
      if (::poll(_pfd,2,-1) > 0) {
	if (_pfd[0].revents & POLLIN)
	  _initialize_client();

	if (_pfd[1].revents & POLLIN) {
	  ShMsg m;
	  if (mq_receive(_shuffleQueue, (char*)&m, sizeof(m), &_priority) < 0) 
	    perror("mq_receive");

	  _copyDatagram(m.dg(), m.msg().bufferIndex());
	  delete m.dg();
	
	  // 	printf("Sending tr %s to mq %d\nmsg %x/%x/%x\n",
	  // 	       TransitionId::name(dgrm.seq.service()), 
	  // 	       _myOutputEvQueue,
	  // 	       _myMsg.bufferIndex(),
	  // 	       _myMsg.numberOfBuffers(),
	  // 	       _myMsg.sizeOfBuffers());


	  if (mq_timedsend(_myOutputEvQueue, (const char*)&m.msg(), sizeof(m.msg()), 0, &_tmo)) {
	    printf("outputEv timedout\n");
	  }
	}
      }
    }
  }

  int init(char *p) 
  { 
    char* shmName    = new char[128];
    char* toQname    = new char[128];
    char* fromQname  = new char[128];

    sprintf(shmName  , "/PdsMonitorSharedMemory_%s",p);
    sprintf(toQname  , "/PdsToMonitorEvQueue_%s",p);
    sprintf(fromQname, "/PdsFromMonitorEvQueue_%s",p);
    _pageSize = (unsigned)sysconf(_SC_PAGESIZE);
  
    int ret = 0;
    _sizeOfShm = (_numberOfEvBuffers + numberofTrBuffers) * _sizeOfBuffers;
    unsigned remainder = _sizeOfShm%_pageSize;
    if (remainder) _sizeOfShm += _pageSize - remainder;

    _mymq_attr.mq_maxmsg  = _numberOfEvBuffers;
    _mymq_attr.mq_msgsize = (long int)sizeof(Msg);
    _mymq_attr.mq_flags   = O_NONBLOCK;

    umask(1);  // try to enable world members to open these devices.

    int shm = shm_open(shmName, OFLAGS, PERMS);
    if (shm < 0) {ret++; perror("shm_open");}

    if ((ftruncate(shm, _sizeOfShm))<0) {ret++; perror("ftruncate");}

    _myShm = (char*)mmap(NULL, _sizeOfShm, PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
    if (_myShm == MAP_FAILED) {ret++; perror("mmap");}

    _flushQueue(_myOutputEvQueue = _openQueue(toQname));

    _flushQueue(_myInputEvQueue  = _openQueue(fromQname));

    sprintf(fromQname, "/PdsFromMonitorDiscovery_%s",p);
    _pfd[0].fd      = _discoveryQueue  = _openQueue(fromQname);
    _pfd[0].events  = POLLIN;
    _pfd[0].revents = 0;
    
    _myOutputTrQueue = new mqd_t[_numberOfClients];
    for(unsigned i=0; i<_numberOfClients; i++) {
      sprintf(toQname  , "/PdsToMonitorTrQueue_%s_%d",p,i);
      _flushQueue(_myOutputTrQueue[i] = _openQueue(toQname));
    }

    struct mq_attr shq_attr;
    shq_attr.mq_maxmsg  = _numberOfEvBuffers;
    shq_attr.mq_msgsize = (long int)sizeof(ShMsg);
    shq_attr.mq_flags   = O_NONBLOCK;
    _shuffleQueue = _openQueue("/PdsShuffleQueue", shq_attr);
    { ShMsg m; _flushQueue(_shuffleQueue,(char*)&m, sizeof(m)); }

    _pfd[1].fd = _shuffleQueue;
    _pfd[1].events  = POLLIN;
    _pfd[1].revents = 0;
      
    _task = new Task(TaskObject("shmcli"));
    _task->call(this);

    // prestuff the input queue which doubles as the free list
    for (int i=0; i<_numberOfEvBuffers; i++) {
      if (mq_send(_myInputEvQueue, (const char *)_myMsg.bufferIndex(i), sizeof(Msg), 0)) 
      { perror("mq_send inQueueStuffing");
	delete this;
	exit(EXIT_FAILURE);
      }
    }

    for(int i=0; i<numberofTrBuffers; i++)
      _freeTr.push(i+_numberOfEvBuffers);

    _pool = new GenericPool(sizeof(ZcpDatagramIterator),2);

    delete[] shmName;
    delete[] toQname;
    delete[] fromQname;

    return ret;
  }

private:  
  void _initialize_client()
  {
    _sem.take();

    Msg msg;
    if (mq_receive(_discoveryQueue, (char*)&msg, sizeof(msg), &_priority) < 0) 
      perror("mq_receive");

    unsigned iclient = msg.bufferIndex();
    printf("_initialize_client %d\n",iclient);

    std::stack<int> tr;
    while(!_cachedTr.empty()) {
      tr.push(_cachedTr.top());
      _cachedTr.pop();
    }
    while(!tr.empty()) {
      int ibuffer = tr.top(); tr.pop();
      _myMsg.bufferIndex(ibuffer);
      
      { Datagram& dgrm = *reinterpret_cast<Datagram*>(_myShm + _sizeOfBuffers * _myMsg.bufferIndex());
	printf("Sending tr %s to mq %d\nmsg %x/%x/%x\n",
	       TransitionId::name(dgrm.seq.service()), 
	       _myOutputTrQueue[iclient],
	       _myMsg.bufferIndex(),
	       _myMsg.numberOfBuffers(),
	       _myMsg.sizeOfBuffers()); }

      if (mq_send(_myOutputTrQueue[iclient], (const char*)&_myMsg, sizeof(_myMsg), 0)) 
	;   // best effort only
      _cachedTr.push(ibuffer);
    }
    _sem.give();
  }

  void _copyDatagram(InDatagram* dg, unsigned index) 
  {
    Datagram& dgrm = dg->datagram();
    if ((dgrm.seq.service() == TransitionId::L1Accept) || (dgrm.seq.service() == TransitionId::Configure))
      PnccdShuffle::shuffle(dgrm);

    _bufferP = _myShm + (_sizeOfBuffers * index);
    //  write the datagram
    memcpy((char*)_bufferP, &dgrm, sizeof(Datagram));
    unsigned offset = sizeof(Datagram);
    //  write the payload
    InDatagramIterator& iter = *dg->iterator(_pool);
    struct iovec iov;
    int remaining = dgrm.xtc.sizeofPayload();
    while(remaining) {
      int isize = iter.read(&iov,1,remaining);
      memcpy(_bufferP+offset, iov.iov_base, iov.iov_len);
      offset += iov.iov_len;
      remaining -= isize;
    }
    delete &iter;
  }

  mqd_t _openQueue(const char* name) { return _openQueue(name,_mymq_attr); }

  mqd_t _openQueue(const char* name, mq_attr& attr) {
    mqd_t q = mq_open(name,  O_CREAT|O_RDWR, PERMS, &attr);
    if (q == (mqd_t)-1) {
      perror("mq_open output");
      printf("mq_attr:\n\tmq_flags 0x%0lx\n\tmq_maxmsg 0x%0lx\n\tmq_msgsize 0x%0lx\n\t mq_curmsgs 0x%0lx\n",
	     attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs );
      fprintf(stderr, "Initializing XTC monitor server encountered an error!\n");
      delete this;
      exit(EXIT_FAILURE);
    }
    else {
      printf("Opened queue %s (%d)\n",name,q);
    }
    return q;
  }

  void _flushQueue(mqd_t q) { Msg m; _flushQueue(q,(char*)&m,sizeof(m)); }

  void _flushQueue(mqd_t q, char* m, unsigned sz) {
    // flush the queues just to be sure they are empty.
    struct mq_attr attr;
    do {
      mq_getattr(q, &attr);
      if (attr.mq_curmsgs)
           mq_receive(q, m, sz, &_priority);
     } while (attr.mq_curmsgs);
  }

  void _push_transition(int ibuffer)
  {
//     { const char* buffer = _myShm + (_sizeOfBuffers * ibuffer);
//       const Datagram& dg = *reinterpret_cast<const Datagram*>(buffer);
//       printf("Pushed %s (%p)\n",TransitionId::name(dg.seq.service()),buffer); }
    _cachedTr.push(ibuffer);
  }

  void _pop_transition()
  {
//     { const char* buffer = _myShm + (_sizeOfBuffers * _cachedTr.top());
//       const Datagram& dg = *reinterpret_cast<const Datagram*>(buffer);
//       printf("Popped %s (%p)\n",TransitionId::name(dg.seq.service()),buffer); }
    _freeTr.push(_cachedTr.top());
    _cachedTr.pop();
  }
      
private:
  Pool* _pool;
  unsigned _sizeOfBuffers;
  int      _numberOfEvBuffers;
  unsigned _numberOfClients;
  unsigned _sizeOfShm;
  char*    _bufferP;   //  pointer to the shared memory area being used
  char*    _myShm; // the pointer to start of shared memory
  mqd_t    _myOutputEvQueue;
  mqd_t    _myInputEvQueue;
  unsigned _priority;
  unsigned _pageSize;
  struct mq_attr _mymq_attr;
  Msg _myMsg;
  mqd_t*   _myOutputTrQueue;
  mqd_t    _discoveryQueue;
  std::stack<int> _cachedTr;
  std::queue<int> _freeTr;
  Task*      _task;
  pollfd     _pfd[2];
  Semaphore  _sem;
  mqd_t          _shuffleQueue;
  timespec       _tmo;
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
  printf("Usage: %s -p <platform> -P <partition> -i <node mask> -n <numb shm buffers> -s <shm buffer size> -c <# clients> [-u <uniqueID>]\n", progname);
}

XtcMonServer* apps;

void sigfunc(int sig_no) {
   delete apps;
   exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {

  unsigned platform=-1UL;
  const char* partition = 0;
  int numberOfBuffers = 0;
  unsigned sizeOfBuffers = 0;
  unsigned nclients = 1;
  unsigned node =  0xffff0;
  char partitionTag[80] = "";
  const char* uniqueID = 0;

  (void) signal(SIGINT, sigfunc);
  if (prctl(PR_SET_PDEATHSIG, SIGINT) < 0) printf("Changing death signal failed!\n");
  int c;
  while ((c = getopt(argc, argv, "p:i:n:P:s:c:u:")) != -1) {
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
    case 'u':
      uniqueID = optarg;
      break;
    case 'n':
      sscanf(optarg, "%d", &numberOfBuffers);
      break;
    case 'P':
      partition = optarg;
      break;
    case 'c':
      nclients = strtoul(optarg, NULL, 0);
      break;
    case 's':
      sizeOfBuffers = (unsigned) strtoul(optarg, NULL, 0);
      break;
    default:
      printf("Unrecogized parameter\n");
      usage(argv[0]);
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
  if (uniqueID) {
    strcat(partitionTag, uniqueID);
    strcat(partitionTag, "_");
  }
  strcat(partitionTag, partition);
  printf("\nPartition Tag:%s\n", partitionTag);


  apps = new XtcMonServer(sizeOfBuffers, numberOfBuffers, nclients);
  apps->init(partitionTag);
  
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

