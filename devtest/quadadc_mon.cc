//
//

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <poll.h>
#include <signal.h>
#include <arpa/inet.h>

#include <string>
#include <vector>

#include "pdsapp/tools/PadMonServer.hh"
#include "pdsdata/app/XtcMonitorServer.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/generic1d.ddl.h"
#include "pdsdata/psddl/bld.ddl.h"

#include "hsd/Module.hh"

extern int optind;

static bool lVerbose = false;
static unsigned nPrint = 20;

class ThreadArgs {
public:
  int fd;
  unsigned busyTime;
  sem_t sem;
  int reqfd;
  int rate;
};

static const int numberofTrBuffers = 8;

namespace Pds {
  class IndirectXtc {
  public:
    Xtc    xtc;
    char*  payload;
  };

  class MyMonitorServer : public XtcMonitorServer {
  public:
    MyMonitorServer(const char* tag,
                    unsigned sizeofBuffers, 
                    unsigned numberofEvBuffers, 
                    unsigned numberofClients) :
      XtcMonitorServer(tag,
                       sizeofBuffers,
                       numberofEvBuffers,
                       numberofClients)
    { 
      _init();
      
      for(unsigned i=0; i<numberofEvBuffers+numberofTrBuffers; i++) {
        _pool.push(new char[sizeofBuffers]);
      }
    }
    ~MyMonitorServer() 
    {
    }
  public:
    void datagram(TransitionId::Value);
    void datagram(TransitionId::Value tr, 
                  const Src&          srcid,
                  const std::vector<IndirectXtc>& xtc);
  private:
    void _deleteDatagram(Dgram* dg) 
    {
      _pool.push((char*)dg); 
    }
  private:
    std::queue<char*> _pool;
  };
};

using namespace Pds;

static const ProcInfo segInfo (Pds::Level::Segment,0,0);
static const DetInfo  srcAdc  (0,DetInfo::XppEndstation,0,DetInfo::Wave8      ,0);
static const BldInfo  srcBld  (0,BldInfo::PhaseCavity);

//
//  Insert a simulated transition
//
void MyMonitorServer::datagram(TransitionId::Value tr)
{
  if (!_pool.empty()) {
    Dgram* dg = (Dgram*)_pool.front(); 
    timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);

    new((void*)&dg->seq) Sequence(Sequence::Event, 
                                  tr, 
                                  ClockTime(tv.tv_sec,tv.tv_nsec), 
                                  TimeStamp(0,0x1ffff,0,0));
    new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), segInfo);
    if (XtcMonitorServer::events(dg) == XtcMonitorServer::Deferred)
      _pool.pop();
  }
  else {
    printf("Transition dropped\n");
  }
}

void MyMonitorServer::datagram(TransitionId::Value tr, 
                               const Src&          srcid,
                               const std::vector<IndirectXtc>& xtc)
{
  if (!_pool.empty()) {
    Dgram* dg = (Dgram*)_pool.front(); 
    timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);

    static unsigned _ievent = 0;
    unsigned v = tr==TransitionId::L1Accept ? _ievent++ : 0;
    new((void*)&dg->seq) Sequence(Sequence::Event, 
                                  tr, 
                                  ClockTime(tv.tv_sec,tv.tv_nsec), 
                                  TimeStamp(0,0x1ffff,v,0));
    Xtc* seg = new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), srcid);
    for(unsigned i=0; i<xtc.size(); i++) {
      Xtc* src = new((char*)seg->alloc(xtc[i].xtc.extent)) Xtc(xtc[i].xtc);
      char* p  = new((char*)src->alloc(xtc[i].xtc.sizeofPayload())) char[xtc[i].xtc.sizeofPayload()];
      memcpy(p,xtc[i].payload,xtc[i].xtc.sizeofPayload());
    }
    if (XtcMonitorServer::events(dg) == XtcMonitorServer::Deferred)
      _pool.pop();
  }
  else {
    printf("Transition dropped\n");
  }
}

void usage(const char* p) {
  printf("Usage: %s [options]\n",p);
  printf("Options:\n");
  printf("\t-I interleave\n");
  printf("\t-R rate     : Set trigger rate [0:929kHz, 1:71kHz, 2:10kHz, 3:1kHz, 4:100Hz, 5:10Hz\n");
  printf("\t-V          : Dump out all events");
}

static Pds::HSD::Module* reg=0;

void sigHandler( int signal ) {
  if (reg) {
    reg->stop();
  }
  
  ::exit(signal);
}

int main(int argc, char** argv) {
  extern char* optarg;
  char evrid='a';
  unsigned length=16;  // multiple of 16
  bool lTest=false;
  Pds::HSD::Module::TestPattern pattern = Pds::HSD::Module::Flash11;
  int interleave = -1;
  unsigned delay=0;
  unsigned prescale=1;
  int adcSyncDelay=-1;

  ThreadArgs args;
  args.fd = -1;
  args.busyTime = 0;
  args.reqfd = -1;
  args.rate = 6;

  int c;
  bool lUsage = false;
  while ( (c=getopt( argc, argv, "I:r:v:A:D:P:S:R:T:Vh")) != EOF ) {
    switch(c) {
    case 'I': interleave = atoi(optarg); break;
    case 'D': delay      = strtoul(optarg,NULL,0); break;
    case 'P': prescale   = strtoul(optarg,NULL,0); break;
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'A':
      adcSyncDelay = strtoul(optarg,NULL,0);
      break;
    case 'S':
      length = strtoul(optarg,NULL,0);
      break;
    case 'R':
      args.rate = strtoul(optarg,NULL,0);
      break;
    case 'T':
      lTest = true;
      pattern = (Pds::HSD::Module::TestPattern)strtoul(optarg,NULL,0);
      break;
    case 'V':
      lVerbose = true;
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    case '?':
    default:
      lUsage = true;
      break;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  MyMonitorServer* _srv = new MyMonitorServer("qadc",
                                             0xA00000,
                                             16,
                                             2);

  //  Insert Map
  _srv->datagram(TransitionId::Map);

  //
  //  Configure channels, event selection
  //
  char dev[16];
  sprintf(dev,"/dev/qadc%c",evrid);
  printf("Using %s\n",dev);

  int fd = open(dev, O_RDWR);
  if (fd<0) {
    perror("Could not open");
    return -1;
  }

  args.fd  = fd;
  sem_init(&args.sem,0,0);

  Pds::HSD::Module* p = reg = Pds::HSD::Module::create(fd);
  if (!p)
    return -2;

  ::signal( SIGINT, sigHandler );

  //
  //  Enabling the test pattern causes a realignment of the clocks
  //   (avoid it, if possible)
  //
  p->disable_test_pattern();
  if (lTest) {
    p->enable_test_pattern(pattern);
  }
  //  p->enable_test_pattern(Module::DMA);

  p->sample_init(length, delay, prescale);
  p->setAdcMux( interleave>=0, (0x11<<interleave));
  p->trig_lcls(args.rate);

  Pds::Bld::BldDataPhaseCavity ecfg;

  {
    //  Create the Generic1DConfig
    unsigned config_length     [8];
    unsigned config_sample_type[8];
    int      config_offset     [8];
    double   config_period     [8];
    unsigned delay = 0;
    unsigned channel = interleave;
    unsigned ps = 1;
      
    for(unsigned i=0; i<8; i++) {
      config_sample_type[i] = Pds::Generic1D::ConfigV0::UINT16;
      config_offset     [i] = double(delay)/119.e6;
      config_length     [i] = (interleave<0 || (i&0x3)==channel) ? length : 0;
      config_period     [i] = (0.8e-9)*(interleave<0 ? double(ps) : 0.25);
    }

    std::vector<Pds::IndirectXtc> xtc(2);

    xtc[0].xtc       = Xtc(TypeId(TypeId::Id_Generic1DConfig,0),
                           srcAdc);
    xtc[0].xtc.alloc(Pds::Generic1D::ConfigV0(8,0,0,0,0)._sizeof());
    xtc[0].payload   = new char[xtc[0].xtc.sizeofPayload()];
    new (xtc[0].payload)
      Pds::Generic1D::ConfigV0(8,
                               config_length,
                               config_sample_type,
                               config_offset,
                               config_period);

    xtc[1].xtc       = Xtc(TypeId(TypeId::Id_PhaseCavity,0),
                           srcBld);
    xtc[1].xtc.alloc(ecfg._sizeof());
    xtc[1].payload   = reinterpret_cast<char*>(&ecfg);

    //  Insert Configure
    _srv->datagram(TransitionId::Configure, 
                   segInfo,
                   xtc);

    delete[] xtc[0].payload;
  }

  _srv->datagram(TransitionId::BeginRun);
  _srv->datagram(TransitionId::BeginCalibCycle);
  _srv->datagram(TransitionId::Enable);

  p->start();

  uint32_t* data = new uint32_t[1<<24];
  
  ssize_t nb;

  std::vector<Pds::IndirectXtc> xtc(2);
  xtc[0].xtc = Xtc(TypeId(TypeId::Id_Generic1DData,0),srcAdc);
  xtc[0].payload = reinterpret_cast<char*>(&data[7]);
  xtc[1].xtc = Xtc(TypeId(TypeId::Id_PhaseCavity,0),srcBld);
  xtc[1].xtc.alloc(ecfg._sizeof());
  xtc[1].payload = reinterpret_cast<char*>(&ecfg);

  while(1) {
    if ((nb = p->read(data, (1<<24)))<0) {
      //      perror("read error");
      //      break;
      //  timeout in driver
      continue;
    }

    xtc[0].xtc.extent = sizeof(Xtc)+data[7]+4;

    _srv->datagram(TransitionId::L1Accept,
                   segInfo,
                   xtc);
  }

  printf("read_thread done\n");

  return 0;
}
