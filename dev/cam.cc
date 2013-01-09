#include "pdsapp/dev/CmdLineTools.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/utility/ToEventWireScheduler.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pds/camera/PimManager.hh"
#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/Opal1kManager.hh"
#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/QuartzManager.hh"
#include "pds/camera/QuartzCamera.hh"
#include "pds/camera/FrameServer.hh"
#include "pds/camera/PicPortCL.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

static bool verbose = false;

static void usage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform> [-g <grabberId>] [-v] [-h]\n",p);
}

static void help()
{
  printf("Options:\n"
         "  -i <detinfo>          integer/integer/integer/integer or string/integer/string/integer\n"
         "                          (e.g. XppEndStation/0/Opal1000/1 or 22/0/3/1)\n"
         "  -p <platform>         platform number\n"
         "  -g <grabberId>        grabber ID (default=0)\n"
         "  -v                    be verbose (default=false)\n"
         "  -h                    help: print this message and exit\n");
}

static Pds::CameraDriver* _driver(int id, const Pds::Src& src)
{
  const Pds::DetInfo info = static_cast<const Pds::DetInfo&>(src);
  switch(info.device()) {
  case Pds::DetInfo::Opal1000:
  case Pds::DetInfo::Opal2000:
  case Pds::DetInfo::Opal4000:
  case Pds::DetInfo::Opal1600:
  case Pds::DetInfo::Opal8000:
    return new PdsLeutron::PicPortCL(*new Pds::Opal1kCamera(info),id);
  case Pds::DetInfo::TM6740  :
    return new PdsLeutron::PicPortCL(*new Pds::TM6740Camera,id);
  case Pds::DetInfo::Quartz4A150:
    return new PdsLeutron::PicPortCL(*new Pds::QuartzCamera(info),id);
  default:
    printf("Unsupported camera %s\n",Pds::DetInfo::name(info.device()));
    exit(1);
    break;
  }
  return NULL;
}

typedef std::list<Pds::Appliance*> AppList;

static void *thread_signals(void*)
{
  while(1) sleep(10);
  return 0;
}

namespace Pds {

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class SegTest : public EventCallback, public SegWireSettings {
  public:
    SegTest(Task*                 task,
	    unsigned              platform,
	    const Src&            src,
	    unsigned              grabberId,
            const AppList&        user_apps) :
      _task     (task),
      _platform (platform),
      _grabberId(grabberId),
      _user_apps(user_apps)
    {
      const Pds::DetInfo info = static_cast<const Pds::DetInfo&>(src);
      switch(info.device()) {
      case DetInfo::Opal1000:
      case DetInfo::Opal2000:
      case DetInfo::Opal4000:
      case DetInfo::Opal1600:
      case DetInfo::Opal8000:
        _camman  = new Opal1kManager(src); break;
      case DetInfo::TM6740  :
        _camman  = new PimManager(src); break;
      case DetInfo::Quartz4A150:
        _camman  = new QuartzManager(src); break;
      default:
        printf("Unsupported camera %s\n",DetInfo::name(info.device()));
        exit(1);
        break;
      }

      _sources.push_back(_camman->server().client());
    }

    virtual ~SegTest()
    {
      delete _camman;

      for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
        delete (*it);

      _task->destroy();
    }

  public:    
    // Implements SegWireSettings
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface)
    {
      wire.add_input(&_camman->server());
    }
    const std::list<Src>& sources() const { return _sources; }
  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("SegTest connected to platform 0x%x\n", 
	     _platform);

      Stream* frmk = streams.stream(StreamParams::FrameWork);

      for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
        (*it)->connect(frmk->inlet());

      _camman->appliance().connect(frmk->inlet());
      _camman->attach(_driver(_grabberId, _sources.front()));
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
    void dissolved(const Node& who)
    {
      const unsigned userlen = 12;
      char username[userlen];
      Node::user_name(who.uid(),username,userlen);
      
      const unsigned iplen = 64;
      char ipname[iplen];
      Node::ip_name(who.ip(),ipname, iplen);
      
      printf("SegTest: platform 0x%x dissolved by user %s, pid %d, on node %s", 
	     who.platform(), username, who.pid(), ipname);
      
      _camman->detach();

      delete this;
    }
    
  private:
    Task*          _task;
    unsigned       _platform;
    CameraManager* _camman;
    int            _grabberId;
    std::list<Src> _sources;
    AppList        _user_apps;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const unsigned NO_PLATFORM = ~0;
  unsigned platform = NO_PLATFORM;
  Arp* arp = 0;

  DetInfo info;
  AppList user_apps;

  unsigned grabberId(0);
  bool infoFlag = false;
  bool helpFlag = false;

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:g:L:vh")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
        usage(argv[0]);
        return -1;
      } else {
        infoFlag = true;
      }
      break;
    case 'p':
      errno = 0;
      endPtr = NULL;
      platform = strtoul(optarg, &endPtr, 0);
      if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
        printf("Error: failed to parse platform number\n");
        usage(argv[0]);
        return -1;
      }
      break;
    case 'g':
      errno = 0;
      endPtr = NULL;
      grabberId = strtoul(optarg, &endPtr, 0);
      if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
        printf("Error: failed to parse grabber ID\n");
        usage(argv[0]);
        return -1;
      }
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
    case 'v':
      verbose = true;
      break;
    case 'h':
      helpFlag = true;
      break;
    }
  }

  if (helpFlag) {
    usage(argv[0]);
    help();
    return 0;
  } else if (!infoFlag || (platform == NO_PLATFORM)) {
    printf("Error: Platform and detinfo required\n");
    usage(argv[0]);
    return 0;
  }

  // launch the SegmentLevel
  if (arp) {
    if (arp->error()) {
      char message[128];
      sprintf(message, "failed to create Arp : %s", 
	      strerror(arp->error()));
      printf("%s %s\n",argv[0], message);
      delete arp;
      return 0;
    }
  }

  printf("Starting handler thread ...\n");
  pthread_t h_thread_signals;
  pthread_create(&h_thread_signals, NULL, thread_signals, 0);

  //
  // Block all signals from this thread (and daughters)
  //
  sigset_t sigset_full;
  sigfillset(&sigset_full);
  pthread_sigmask(SIG_BLOCK, &sigset_full, 0);

  Task* task = new Task(Task::MakeThisATask);

  Node node(Level::Source,platform);
  info = DetInfo(node.pid(), info.detector(), info.detId(), info.device(), info.devId());

  SegTest* segtest = new SegTest(task, 
				 platform, 
                                 info,
				 grabberId,
                                 user_apps);

  printf("Creating segment level ...\n");
  SegmentLevel* segment = new SegmentLevel(platform, 
					   *segtest,
					   *segtest, 
					   arp);
  if (segment->attach()) {
    task->mainLoop();
  }

  segment->detach();
  if (arp) delete arp;
  return 0;
}
