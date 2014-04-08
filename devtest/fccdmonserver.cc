#include "pdsdata/app/XtcMonitorServer.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pds/udpcam/Fccd960Reorder.hh"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/config/FccdConfigType.hh"

#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <dirent.h>
#include <string.h>
#include <string>
#include <list>
#include <queue>

using namespace Pds;
using std::string;
using std::list;
using std::queue;
using Pds::Dgram;


void usage(char* progname) {
  std::cerr << "Usage: " << progname
            << " (-f <filename> | -l <filename_list> | -x <run_file_prefix> | -d <xtc_dir>)" // choose one
            << " -p <partitionTag> -n <numberOfBuffers> -s <sizeOfBuffers>" // mandatory
            << " [-r <ratePerSec>] [-c <# clients>] [-S <sequence length>]" // optional
            << " [-L] [-v] [-V]" // debugging (optional)
            << std::endl;
}

static const unsigned ReorderedSize = 960*960*2;
//static const unsigned Fudge         = 13*192*2;
static int      Fudge         = 0;
static unsigned RawSize       = ReorderedSize+7*192*2+Fudge;
static TypeId _dataType(TypeId::Id_Frame,1);
static DetInfo _src(0,DetInfo::NoDetector,0,DetInfo::Fccd960,0);

class MyDataFile {
public:
  MyDataFile(const char* fname) { 
    _f = fopen(fname,"r"); 
    _buff=new unsigned char[RawSize];
    fccd960Initialize(_chanMap, _topBot);
  }
  ~MyDataFile() { fclose(_f); delete[] _buff; }
public:
  Dgram* next(char* b) {
    int sz = fread(_buff,1,RawSize,_f);
    if (sz != int(RawSize)) return 0;

    Transition tr(TransitionId::L1Accept,Env(0));
    Dgram* dg = (Dgram*)b;
    new ((char*)&dg->seq) Sequence(tr.sequence());
    new ((char*)&dg->env) Env(tr.env());
    new ((char*)&dg->xtc) Xtc(_xtcType,Src(Level::Event));
    Xtc* xtc = new((char*)dg->xtc.alloc(sizeof(Xtc)+ReorderedSize+sizeof(Camera::FrameV1)))
      Xtc(_dataType,_src);
    new(xtc->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(960,960,16,0);

    fccd960Reorder(_chanMap, _topBot, _buff+Fudge, (uint16_t *)xtc->alloc(ReorderedSize));
    return dg;
  }
private:
  FILE* _f;
  unsigned char* _buff;
  enum { mapLength = 192};
  uint16_t _chanMap[mapLength];
  uint16_t _topBot [mapLength];
};

//#define CLOCK CLOCK_PROCESS_CPUTIME_ID
#define CLOCK CLOCK_REALTIME

static void printTransition(const Dgram* dg) {
  printf("%18s transition: time %08x/%08x, payloadSize 0x%08x dmg 0x%x\n",
         TransitionId::name(dg->seq.service()),
         dg->seq.stamp().fiducials(),
         dg->seq.stamp().ticks(),
         dg->xtc.sizeofPayload(),
         dg->xtc.damage.value());
}

class MyMonitorServer : public XtcMonitorServer {
private:
  queue<Dgram*> _pool;

  void _deleteDatagram(Dgram* dg) {
  }

public:
  MyMonitorServer(const char* tag,
                  unsigned sizeofBuffers, 
                  unsigned numberofEvBuffers, 
                  unsigned numberofClients,
                  unsigned sequenceLength) :
    XtcMonitorServer(tag,
                     sizeofBuffers,
                     numberofEvBuffers,
                     numberofClients,
                     sequenceLength) {
    // Only need these buffers for inserted transitions { Map, Unconfigure, Unmap }
    unsigned depth = 4;
    for(unsigned i=0; i<depth; i++)
      _pool.push(reinterpret_cast<Dgram*>(new char[sizeofBuffers]));
  }

  ~MyMonitorServer() {
    while(!_pool.empty()) {
      delete _pool.front();
      _pool.pop();
    }
  }

  Dgram* pop() { 
    Dgram* dg = _pool.front();
    _pool.pop();
    return dg;
  }

  void push(Dgram* dg) { _pool.push(dg); }

  XtcMonitorServer::Result events(Dgram* dg) {
    Xtc xtc = dg->xtc;
    if (XtcMonitorServer::events(dg) == XtcMonitorServer::Handled) {
      _deleteDatagram(dg);
    }
    return XtcMonitorServer::Deferred;
  }

  // Insert a simulated transition
  void insert(TransitionId::Value tr) {
    Dgram* dg = _pool.front(); 
    _pool.pop(); 
    new((void*)&dg->seq) Sequence(Sequence::Event, tr, ClockTime(0,0), TimeStamp(0,0,0,0));
    new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0),ProcInfo(Level::Event,0,0));
    ::printTransition(dg);
    events(dg);
    _pool.push(dg);
  }
};

class XtcRunSet {
private:
  MyMonitorServer* _server;
  list<string> _paths;
  char* _dg;
  long long int _period;
  bool _verbose;
  bool _veryverbose;
  void _addPaths(list<string> newPaths);
  double timeDiff(struct timespec* end, struct timespec* start);

public:
  XtcRunSet();
  void addSinglePath(string path);
  void addPathsFromListFile(string listFile);
  void connect(char* partitionTag, unsigned sizeOfBuffers, int numberOfBuffers, unsigned nclients, unsigned sequenceLength, int rate, bool verbose = false, bool veryverbose = false);
  void run();
};

// Takes a new list of paths, sorts it, and then appends the contents
// to the existing list of paths (by moving the new paths).
void XtcRunSet::_addPaths(list<string> newPaths) {
  newPaths.sort();
  _paths.splice(_paths.end(), newPaths);
}

double XtcRunSet::timeDiff(struct timespec* end, struct timespec* start) {
  double diff;
  diff =  double(end->tv_sec - start->tv_sec) * 1.e9;
  diff += double(end->tv_nsec);
  diff -= double(start->tv_nsec);
  return diff;
}

// Constructor that starts with an empty list of paths.
XtcRunSet::XtcRunSet() :
  _server(NULL),
  _dg(NULL) {
}

void XtcRunSet::addSinglePath(string path) {
  _paths.push_back(path);
}

// Add every file listed in the list file.
void XtcRunSet::addPathsFromListFile(string listFile) {
  std::cout << "addPathsFromListFile(" << listFile << ")" << std::endl;
  FILE *flist = fopen(listFile.c_str(), "r");
  if (flist == NULL) {
    perror(listFile.c_str());
    return;
  }
  list<string> newPaths;
  char filename[FILENAME_MAX];
  while (fscanf(flist, "%s", filename) != EOF) {
    newPaths.push_back(filename);
  }
  _addPaths(newPaths);
}

void XtcRunSet::connect(char* partitionTag, unsigned sizeOfBuffers, int numberOfBuffers, unsigned nclients, unsigned sequenceLength, int rate, bool verbose, bool veryverbose) {
  if (_dg) delete[] _dg;
  _dg = new char[sizeOfBuffers];

  if (_server == NULL) {
    _verbose = verbose;
    _veryverbose = veryverbose;

    if (rate > 0) {
      _period = 1000000000 / rate; // period in nanoseconds
      std::cout << "Rate is " << rate << " Hz; period is " << _period / 1e6 << " msec" << std::endl;
    } else {
      _period = 0;
      std::cout << "Rate was not specified; will run unthrottled." << std::endl;
    }

    struct timespec start, now;
    clock_gettime(CLOCK, &start);
    _server = new MyMonitorServer(partitionTag,
                                  sizeOfBuffers, 
                                  numberOfBuffers, 
                                  nclients,
                                  sequenceLength);
    clock_gettime(CLOCK, &now);
    printf("Opening shared memory took %.3f msec.\n", timeDiff(&now, &start) / 1e6);
  }
}

void XtcRunSet::run() {

  _server->insert(TransitionId::Map);

  { Dgram* dg = _server->pop();
    new((void*)&dg->seq) Sequence(Sequence::Event, TransitionId::Configure,
                                  ClockTime(0,0), TimeStamp(0,0,0,0));
    Xtc* extc = new((char*)dg->xtc.alloc(sizeof(Xtc))) Xtc(TypeId(TypeId::Id_Xtc,0),ProcInfo(Level::Event,0,0));
    { Xtc* sxtc = new((char*)extc->next())  Xtc(_fccdConfigType,_src);
      float    fdummy [32]; memset( fdummy,0,32*sizeof(float));
      uint16_t usdummy[32]; memset(usdummy,0,32*sizeof(uint16_t));
      sxtc->extent += (new ((char*)sxtc->next()) FccdConfigType(0, true, false, 0,
                                                         fdummy, usdummy))->_sizeof();
      extc->extent += sxtc->extent;
      dg->xtc.extent += sxtc->extent; }
    ::printTransition(dg);
    _server->events(dg);
    _server->push(dg);
  }

  _server->insert(TransitionId::BeginRun);
  _server->insert(TransitionId::BeginCalibCycle);

  timespec loopStart;
  clock_gettime(CLOCK, &loopStart);
  int dgCount = 0;

  for(list<string>::iterator it=_paths.begin();
      it!=_paths.end(); it++) {
    MyDataFile* stream = new MyDataFile(it->c_str());
    Dgram* dg;
    while ((dg = stream->next(_dg)) != NULL) {
      timespec dgStart;
      clock_gettime(CLOCK, &dgStart);
      dgCount++;
      _server->events(dg);
      if (dg->seq.service() != TransitionId::L1Accept) {
        printTransition(dg);
        clock_gettime(CLOCK, &loopStart);
        dgCount = 0;
      } else if (_verbose) {
        timespec now;
        clock_gettime(CLOCK, &now);
        double hz = double(dgCount) / (timeDiff(&now, &loopStart) / 1.e9);
        printf("%18s transition: time %08x/%08x, payloadSize 0x%08x, avg rate %8.3f Hz%c",
               TransitionId::name(dg->seq.service()),
               dg->seq.stamp().fiducials(),dg->seq.stamp().ticks(),
               dg->xtc.sizeofPayload(), hz,
               _veryverbose ? '\n' : '\r');
      }
      
      if (_period != 0) {
        timespec now;
        clock_gettime(CLOCK, &now);
        long long int busyTime = timeDiff(&now, &dgStart);
        if (_period > busyTime) {
          timespec sleepTime;
          sleepTime.tv_sec = 0;
          sleepTime.tv_nsec = _period - busyTime;
          if (nanosleep(&sleepTime, &now) < 0) {
            perror("nanosleep");
          }
        }
      }
    }
    delete stream;
  }

  _server->insert(TransitionId::EndCalibCycle);
  _server->insert(TransitionId::EndRun);
  _server->insert(TransitionId::Unconfigure);
  _server->insert(TransitionId::Unmap);
}



int main(int argc, char* argv[]) {
  // Exactly one of these values must be supplied
  char* xtcFile = NULL;
  char* listFile = NULL;
  char* runPrefix = NULL;
  char* xtcDir = NULL;

  // These are mandatory
  int numberOfBuffers = 4;
  unsigned sizeOfBuffers = 0x900000;
  char* partitionTag = NULL;

  // These are optional
  int rate = 60; // Hz
  unsigned nclients = 1;
  unsigned sequenceLength = 1;

  // These are for debugging (also optional)
  bool loop = false;
  bool verbose = false;
  bool veryverbose = false;

  //  (void) signal(SIGINT, sigfunc);
  //  (void) signal(SIGSEGV, sigfunc);

  int c;
  while ((c = getopt(argc, argv, "f:l:x:d:p:n:s:r:c:S:F:LvVh?")) != -1) {
    switch (c) {
      case 'F':
        Fudge = atoi(optarg); 
        break;
      case 'f':
        xtcFile = optarg;
        break;
      case 'l':
        listFile = optarg;
        break;
      case 'x':
        runPrefix = optarg;
        break;
      case 'd':
        xtcDir = optarg;
        break;
      case 'n':
        sscanf(optarg, "%d", &numberOfBuffers);
        break;
      case 's':
        sizeOfBuffers = (unsigned) strtoul(optarg, NULL, 0);
        break;
      case 'r':
        sscanf(optarg, "%d", &rate);
        break;
      case 'p':
        partitionTag = optarg;
        break;
      case 'c':
        nclients = strtoul(optarg, NULL, 0);
        break;
      case 'S':
        sscanf(optarg, "%d", &sequenceLength);
        break;
      case 'L':
        loop = true;
        printf("Enabling infinite looping\n");
        break;
      case 'v':
        verbose = true;
        break;
      case 'V':
        verbose = true;
        veryverbose = true;
        break;
      case 'h':
      case '?':
        usage(argv[0]);
        exit(0);
      default:
        fprintf(stderr, "Unrecognized option -%c!\n", c);
        usage(argv[0]);
        exit(0);
    }
  }

  if (sizeOfBuffers == 0 || numberOfBuffers == 0) {
    std::cerr << "Must specify both size (-s) and number (-n) of buffers." << std::endl;
    usage(argv[0]);
    exit(2);
  }
  if (partitionTag == NULL) {
    partitionTag = getenv("USER");
    if (partitionTag == NULL) {
      std::cerr << "Must specify a partition tag." << std::endl;
      usage(argv[0]);
      exit(2);
    }
  }
  int choiceCount = (xtcFile != NULL) + (listFile != NULL) + (runPrefix != NULL) + (xtcDir != NULL);
  if (choiceCount != 1) {
    std::cerr << "Must specify exactly one of -f, -l, -r, -d. You specified " << choiceCount << std::endl;
    usage(argv[0]);
    exit(2);
  }

  XtcRunSet runSet;
  runSet.connect(partitionTag, sizeOfBuffers, numberOfBuffers, nclients, sequenceLength, rate, verbose, veryverbose);
  do {
    if (xtcFile) {
      runSet.addSinglePath(xtcFile);
    } else if (listFile) {
      runSet.addPathsFromListFile(listFile);
    }
    runSet.run();
  } while (loop);
}
