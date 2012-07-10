#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"

#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static bool verbose = false;
static const unsigned CfgSize = sizeof(TM6740ConfigType);
static const unsigned FexSize = sizeof(FrameFexConfigType);

using namespace Pds;

class SimApp : public Appliance {
public:
  SimApp(const Src& src, unsigned evtsz) :
    _cfgtc(_tm6740ConfigType,src), 
    _cfgpayload(new char[CfgSize]),
    _fextc(_frameFexConfigType,src), 
    _fexpayload(new char[FexSize]),
    _evttc(TypeId(TypeId::Any,0),src),
    _evtpayload(new char[evtsz])
  {
    _cfgtc.extent += CfgSize;
    new(_cfgpayload)TM6740ConfigType(0x20,0x20,100,100,false,
                                     TM6740ConfigType::Ten_bit,
                                     TM6740ConfigType::x1,
                                     TM6740ConfigType::x1,
                                     TM6740ConfigType::Linear);
    _fextc.extent += FexSize;
    new(_fexpayload)FrameFexConfigType(FrameFexConfigType::FullFrame,
                                       1,
                                       FrameFexConfigType::NoProcessing,
                                       Pds::Camera::FrameCoord(0,0),
                                       Pds::Camera::FrameCoord(TM6740ConfigType::Column_Pixels,
                                                               TM6740ConfigType::Row_Pixels),
                                       0,
                                       0, NULL);
    _evttc.extent += evtsz;
  }
  ~SimApp() 
  {
    delete[] _cfgpayload;
    delete[] _evtpayload;
  }
public:
  Transition* transitions(Transition* tr) 
  { 
    switch(tr->id()) {
    case TransitionId::Configure:
      break;
    case TransitionId::L1Accept:
      break;
    default:
      break;
    }
    return tr; 
  }
  InDatagram* events     (InDatagram* dg) 
  {
    switch(dg->seq.service()) {
    case TransitionId::Configure:
      dg->insert(_fextc,_cfgpayload);
      dg->insert(_cfgtc,_cfgpayload);
      break;
    case TransitionId::L1Accept:
      dg->insert(_evttc,_evtpayload);
      break;
    default:
      break;
    }
    return dg; 
  }
private:
  Xtc   _cfgtc;
  char* _cfgpayload;
  Xtc   _fextc;
  char* _fexpayload;
  Xtc   _evttc;
  char* _evtpayload;
};

//
//  Implements the callbacks for attaching/dissolving.
//  Appliances can be added to the stream here.
//
class SegTest : public EventCallback, public SegWireSettings {
public:
  SegTest(Task*                 task,
          unsigned              platform,
          const Src&            src,
          unsigned              evtsz) :
    _task    (task),
    _platform(platform),
    _app     (new SimApp(src, evtsz))
  {
    _sources.push_back(src);
  }

  virtual ~SegTest()
  {
    delete _app;
    _task->destroy();
  }

public:    
  // Implements SegWireSettings
  void connect (InletWire&, StreamParams::StreamType, int) {}
  const std::list<Src>& sources() const { return _sources; }
private:
  // Implements EventCallback
  void attached(SetOfStreams& streams)
  {
    printf("SegTest connected to platform 0x%x\n", _platform);

    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _app->connect(frmk->inlet());
  }
  void failed(Reason reason)
  {
    static const char* reasonname[] = { "platform unavailable", 
                                        "crates unavailable", 
                                        "fcpm unavailable" };
    printf("SegTest: unable to allocate crates on platform 0x%x : %s\n", 
           _platform, reasonname[reason]);
    delete this;
  }
  void dissolved(const Node& who) { delete this; }
    
private:
  Task*          _task;
  unsigned       _platform;
  std::list<Src> _sources;
  SimApp*        _app;
};

void printUsage(char* s) {
  printf( "Usage: %s [-h] [-d <detector>] [-i <deviceID>] -p <platform>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -d      Set detector type by name [Default: NoDetector]\n"
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -z      Set event size in bytes\n"
      "    -v      Toggle verbose mode\n",
      s
  );
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = -1UL;

  DetInfo::Detector   detector            = DetInfo::NoDetector;
  unsigned detid = 0;
  unsigned devid = 0;
  unsigned evtsz = 1<<20;
  unsigned index;

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "d:i:p:z:vh")) != EOF ) {
    bool     found;
    switch(c) {
      case 'd':
      found = false;
      for (index=0; !found && index<DetInfo::NumDetector; index++) {
        if (!strcmp(optarg, DetInfo::name((DetInfo::Detector)index))) {
          detector = (DetInfo::Detector)index;
          found = true;
        }
      }
      if (!found) {
        printf("Bad Detector name: %s\n  Detector Names are:\n", optarg);
        for (index=0; index<DetInfo::NumDetector; index++) {
          printf("\t%s\n", DetInfo::name((DetInfo::Detector)index));
        }
        printUsage(argv[0]);
        return 0;
      }
      break;
    case 'i':
      devid  = strtoul(endPtr+1, &endPtr, 0);
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'z':
      evtsz = strtoul(optarg, NULL, 0);
      break;
    case 'v':
      verbose = !verbose;
      printf("Verbose mode now %s\n", verbose ? "true" : "false");
      break;
    case 'h':
      printUsage(argv[0]);
      return 0;
    default:
      printf("Option %c not understood\n", (char) c&0xff);
      printUsage(argv[0]);
      return 1;
    }
  }

  if (platform == -1UL) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,platform);
  SegTest* segtest = new SegTest(task, 
         platform, 
         DetInfo(node.pid(), 
           detector, detid,
           DetInfo::TM6740, devid),
                                 evtsz);
  SegmentLevel* segment = new SegmentLevel(platform, 
             *segtest,
             *segtest, 
             NULL);
  if (segment->attach()) {
    task->mainLoop();
  }

  segment->detach();

  return 0;
}
