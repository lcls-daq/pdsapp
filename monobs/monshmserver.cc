#include "pdsapp/tools/PnccdShuffle.hh"
#include "pdsapp/tools/CspadShuffle.hh"

#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/ObserverLevel.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/service/GenericPool.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/app/XtcMonitorServer.hh"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>

using namespace Pds;

class LiveMonitorServer : public Appliance,
                          public XtcMonitorServer {
public:
  LiveMonitorServer(const char* tag,
                    unsigned sizeofBuffers,
                    int numberofEvBuffers,
                    unsigned numberofEvQueues) :
    XtcMonitorServer(tag, sizeofBuffers, numberofEvBuffers, numberofEvQueues),
    _pool           (new GenericPool(sizeof(ZcpDatagramIterator),2)),
    _upool          (new GenericPool(sizeof(CDatagram),1))
  {
  }
  ~LiveMonitorServer()
  {
    delete _pool;
    delete _upool;
  }

public:
  Transition* transitions(Transition* tr)
  {
    printf("LiveMonitorServer tr %s\n",TransitionId::name(tr->id()));
    if (tr->id() == TransitionId::Unmap) {
      //
      //  Generate an Unmap datagram
      //
      CDatagram* unmap = new (_upool) CDatagram(Datagram(*tr, TypeId(TypeId::Id_Xtc,0),ProcInfo(Level::Observer,getpid(),0)));
      Dgram& dgrm = reinterpret_cast<Dgram&>(unmap->datagram());
      XtcMonitorServer::events(&dgrm);
      delete unmap;
      
      //      _pop_transition();
    }
    return tr;
  }

  InDatagram* occurrences(InDatagram* dg) { return dg; }

  InDatagram* events     (InDatagram* dg)
  {
    Dgram& dgrm = reinterpret_cast<Dgram&>(dg->datagram());
    return
      (XtcMonitorServer::events(&dgrm) == XtcMonitorServer::Deferred) ?
      (InDatagram*)DontDelete : dg;
  }

private:
  void _copyDatagram(Dgram* dg, char* b)
  {
    Datagram& dgrm = *reinterpret_cast<Datagram*>(dg);
    InDatagram* indg = static_cast<InDatagram*>(&dgrm);

    PnccdShuffle::shuffle(dgrm);
    CspadShuffle::shuffle(reinterpret_cast<Dgram&>(dgrm));

    //  write the datagram
    memcpy(b, &dgrm, sizeof(Datagram));
    //  write the payload
    InDatagramIterator& iter = *indg->iterator(_pool);
    iter.copy(b+sizeof(Datagram), dgrm.xtc.sizeofPayload());
    delete &iter;
  }

  void _deleteDatagram(Dgram* dg)
  {
    Datagram& dgrm = *reinterpret_cast<Datagram*>(dg);
    InDatagram* indg = static_cast<InDatagram*>(&dgrm);
    delete indg;
  }
private:
  Pool* _pool;
  Pool* _upool;
};

#include "pds/utility/EbBase.hh"

class MyEbDump : public Appliance {
public:
  MyEbDump(InletWire* eb) : _eb(static_cast<EbBase*>(eb)) {}
public:
  Transition* transitions(Transition* in) { return in; }
  InDatagram* occurrences(InDatagram* in) { return in; }
  InDatagram* events     (InDatagram* in) {
    const Datagram& dg = in->datagram();
    if (dg.seq.service()==TransitionId::EndRun)
      _eb->dump(1);
    return in;
  }
private:
  EbBase* _eb;
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
    (new MyEbDump(streams.wire()))->connect(frmk->inlet());
  }
  void failed   (Reason reason)   { _task->destroy(); delete this; }
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  Appliance*  _appliances;
};

void usage(char* progname) {
  printf("Usage: %s -p <platform> -P <partition> -i <node mask> -n <numb shm buffers> -s <shm buffer size> [-c|-q <# event queues>] [-d] [-u <uniqueID>] [-h]\n", progname);
}

LiveMonitorServer* apps;

void sigfunc(int sig_no) {
  static bool _handled=false;
  if (!_handled) {
    _handled = true;
    printf("handling signal %d\n",sig_no);
    if (apps) {
      apps->unlink();
      apps = 0;
      printf("done with signal %d\n",sig_no);
      exit(EXIT_SUCCESS);
    }
  }
}

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = unsigned(-1UL);
  unsigned platform=NO_PLATFORM;
  const char* partition = 0;
  int numberOfBuffers = 0;
  unsigned sizeOfBuffers = 0;
  unsigned nevqueues = 1;
  unsigned node =  0xffff0;
  char partitionTag[80] = "";
  const char* uniqueID = 0;
  bool ldist = false;
  Appliance* uapps = 0;
  int slowReadout = 0;

  struct sigaction int_action;

  int_action.sa_handler = sigfunc;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;

#define REGISTER(t) {                                           \
    if (sigaction(t, &int_action, 0) > 0)                       \
      printf("Couldn't set up #t handler\n");                   \
  }

  REGISTER(SIGINT);
  REGISTER(SIGKILL);
  REGISTER(SIGSEGV);
  REGISTER(SIGABRT);
  REGISTER(SIGTERM);

#undef REGISTER

  int c;
  while ((c = getopt(argc, argv, "p:i:n:P:s:c:q:u:L:w:dh")) != -1) {
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
    case 'q':
      nevqueues = strtoul(optarg, NULL, 0);
      break;
    case 's':
      sizeOfBuffers = (unsigned) strtoul(optarg, NULL, 0);
      break;
    case 'd':
      ldist = true;
      break;
    case 'w':
      slowReadout = strtoul(optarg, NULL, 0);
      break;
    case 'L':
      { for(const char* p = strtok(optarg,","); p!=NULL; p=strtok(NULL,",")) {
    printf("dlopen %s\n",p);

    void* handle = dlopen(p, RTLD_LAZY);
    if (!handle) {
      printf("dlopen failed : %s\n",dlerror());
      break;
    }

    // reset errors
    const char* dlsym_error;
    dlerror();

    // load the symbols
    create_app* c_user = (create_app*) dlsym(handle, "create");
    if ((dlsym_error = dlerror())) {
      fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
      break;
    }
    if (uapps)
      c_user()->connect(uapps);
    else
      uapps = c_user();
  }
  break;
      }
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

  apps = new LiveMonitorServer(partitionTag,sizeOfBuffers, numberOfBuffers, nevqueues);
  apps->distribute(ldist);

  if (uapps)
    apps->connect(uapps);
  else
    uapps = apps;

  if (apps) {
    Task* task = new Task(Task::MakeThisATask);
    MyCallback* display = new MyCallback(task,
           uapps);

    ObserverLevel* event = new ObserverLevel(platform,
                                             partition,
                                             node,
                                             *display,
                                             slowReadout,
                                             sizeOfBuffers);

    if (event->attach()) {
      task->mainLoop();
      event->detach();
      delete uapps;
      delete event;
      delete display;
    }
    else {
      printf("Observer failed to attach to platform\n");
      delete uapps;
      delete event;
    }
  } else {
    printf("%s: Error creating LiveMonitorServer\n", __FUNCTION__);
  }
  return 0;
}

