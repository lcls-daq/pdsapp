#include "pdsapp/dev/CmdLineTools.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/utility/ToEventWireScheduler.hh"

#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/QuartzConfigType.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/camera/FrameV1.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <dlfcn.h>
#include <new>

static bool verbose = false;
static const unsigned CfgSize = 0x1000;
static const unsigned FexSize = 0x1000;
static const unsigned EvtSize = 0x1000000; // 16MB
static int ndrop = 0;
static int ntime = 0;
static double ftime = 0;

typedef std::list<Pds::Appliance*> AppList;

using namespace Pds;
using Pds::Camera::FrameV1;

class SimApp : public Appliance {
public:
  SimApp(const Src& src) 
  {
    _cfgpayload = new char[CfgSize];

    _evtpayload = new char[EvtSize];
    _evttc = new(_evtpayload) Xtc(TypeId(TypeId::Id_Frame,1),src);

    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Opal1000:
    case DetInfo::Opal2000:
    case DetInfo::Opal4000:
    case DetInfo::Opal1600:
    case DetInfo::Opal8000:
      _cfgtc = new(_cfgpayload) Xtc(_opal1kConfigType,src);
      _cfgtc->extent += (new (_cfgtc->next()) Opal1kConfigType(32, 100, 
							       Opal1kConfigType::Twelve_bit,
							       Opal1kConfigType::x1,
							       Opal1kConfigType::None,
							       true,
							       false))->size();
      _evttc->extent += (new(_evttc->next()) FrameV1(Opal1kConfigType::max_column_pixels(info),
						     Opal1kConfigType::max_row_pixels(info),
						     12, 32))->data_size()+sizeof(FrameV1);
      break;
    case DetInfo::TM6740:
      _cfgtc = new(_cfgpayload) Xtc(_tm6740ConfigType,src);
      _cfgtc->extent += sizeof(*new (_cfgtc->next()) TM6740ConfigType(32, 32, 100, 100, false,
                                                                      TM6740ConfigType::Ten_bit,
                                                                      TM6740ConfigType::x1,
                                                                      TM6740ConfigType::x1,
                                                                      TM6740ConfigType::Linear));
      _evttc->extent += (new (_evttc->next()) FrameV1(TM6740ConfigType::Column_Pixels,
                                                      TM6740ConfigType::Row_Pixels,
                                                      10, 32))->data_size()+sizeof(FrameV1);
      break;
    case DetInfo::Quartz4A150:
      _cfgtc = new(_cfgpayload) Xtc(_quartzConfigType,src);
      _cfgtc->extent += (new (_cfgtc->next()) QuartzConfigType(32, 100, 
                                                               QuartzConfigType::Eight_bit,
                                                               QuartzConfigType::x1,
                                                               QuartzConfigType::x1,
                                                               QuartzConfigType::None,
                                                               false))->size();
      _evttc->extent += (new (_evttc->next()) FrameV1(QuartzConfigType::Column_Pixels,
                                                      QuartzConfigType::Row_Pixels,
                                                      8, 32))->data_size()+sizeof(FrameV1);
      break;
    default:
      printf("Unsupported camera %s\n",Pds::DetInfo::name(info.device()));
      exit(1);
    }

    _fexpayload = new char[FexSize];
    _fextc = new(_fexpayload) Xtc(_frameFexConfigType,src);
    new(_fextc->next()) FrameFexConfigType(FrameFexConfigType::FullFrame,
					   1,
					   FrameFexConfigType::NoProcessing,
					   Pds::Camera::FrameCoord(0,0),
					   Pds::Camera::FrameCoord(0,0),
					   0,
					   0, NULL);
    _fextc->extent += sizeof(FrameFexConfigType);
  }
  ~SimApp() 
  {
    delete _cfgtc;
    delete _fextc;
    delete _evttc;
    delete[] _cfgpayload;
    delete[] _fexpayload;
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
      dg->insert(*_cfgtc,_cfgtc->payload());
      dg->insert(*_fextc,_fextc->payload());
      break;
    case TransitionId::L1Accept:
    {
      static int itime=0;
      if ((++itime)==ntime) {
	itime = 0;
	timeval ts = { int(ftime), int(drem(ftime,1)*1000000) };
	select( 0, NULL, NULL, NULL, &ts);
      }

      static int idrop=0;
      if (++idrop==ndrop) {
	idrop=0;
	return 0;
      }

      dg->insert(*_evttc,_evttc->payload());
      break;
    }
    default:
      break;
    }
    return dg; 
  }
private:
  Xtc*  _cfgtc;
  char* _cfgpayload;
  Xtc*  _fextc;
  char* _fexpayload;
  Xtc*  _evttc;
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
          const AppList&        user_apps) :
    _task     (task),
    _platform (platform),
    _app      (new SimApp(src)),
    _user_apps(user_apps)
  {
    _sources.push_back(src);
  }

  virtual ~SegTest()
  {
    delete _app;

    for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
      delete (*it);

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

    for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
      (*it)->connect(frmk->inlet());

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
  AppList        _user_apps;
};

void printUsage(char* s) {
  printf( "Usage: %s [-h] [-d <detector>] [-i <deviceID>] -p <platform>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -i      Set device info\n"
      "    -v      Toggle verbose mode\n"
      "    -D <N>  Drop every N events\n"
      "    -T <S>,<N>  Delay S seconds every N events\n",
      s
  );
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned platform = -1UL;

  DetInfo info;
  AppList user_apps;

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "i:p:vD:T:L:h")) != EOF ) {
    bool     found;
    switch(c) {
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
        printUsage(argv[0]);
        return -1;
      }
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'v':
      verbose = !verbose;
      printf("Verbose mode now %s\n", verbose ? "true" : "false");
      break;
    case 'D':
      ndrop = strtoul(optarg, NULL, 0);
      break;
    case 'T':
      ntime = strtoul(optarg, &endPtr, 0);
      ftime = strtod (endPtr+1, &endPtr);
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
          user_apps.push_back( c_user() );
        }
        break;
      }
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
  info = DetInfo(node.pid(), info.detector(), info.detId(), info.device(), info.devId());
  SegTest* segtest = new SegTest(task, 
                                 platform, 
                                 info,
                                 user_apps);

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
