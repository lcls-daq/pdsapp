#include "pds/service/CmdLineTools.hh"
#include "pds/client/FrameCompApp.hh"

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
#include "pds/camera/OrcaManager.hh"
#include "pds/camera/OrcaCamera.hh"
#include "pds/camera/QuartzManager.hh"
#include "pds/camera/QuartzCamera.hh"
#include "pds/camera/FrameServer.hh"
#include "pds/camera/EdtPdvCL.hh"

#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/QuartzConfigType.hh"
#include "pds/config/OrcaConfigType.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

static bool verbose = false;

static void usage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> [OPTIONS]\n", p);
  printf("\n"
         "Options:\n"
         "    -i <detinfo>                int/int/int/int or string/int/string/int\n"
         "                                  (e.g. XppEndStation/0/Opal1000/1 or 22/0/3/1)\n"
         "    -p <platform>,<mod>,<chan>  platform number, EVR module, EVR channel\n"
         "    -g <grabberId>              grabber ID (default=0)\n"
         "    -c <channel>                EDT channel number (default=0)\n"
         "    -C <N>                      compress and copy every Nth event\n"
         "    -t                          generate test pattern\n"
         "    -v                          be verbose (default=false)\n"
         "    -u <alias>                  set device alias\n"
         "    -h                          print this message and exit\n");
}

static Pds::CameraDriver* _driver(int id, int channel, const Pds::Src& src)
{
  const Pds::DetInfo info = static_cast<const Pds::DetInfo&>(src);
  switch(info.device()) {
  case Pds::DetInfo::Opal1000:
  case Pds::DetInfo::Opal2000:
  case Pds::DetInfo::Opal4000:
  case Pds::DetInfo::Opal1600:
  case Pds::DetInfo::Opal8000:
    return new Pds::EdtPdvCL(*new Pds::Opal1kCamera(info),id,channel);
  case Pds::DetInfo::TM6740  :
    return new Pds::EdtPdvCL(*new Pds::TM6740Camera,id,channel);
  case Pds::DetInfo::Quartz4A150:
    return new Pds::EdtPdvCL(*new Pds::QuartzCamera(info),id,channel);
  case Pds::DetInfo::OrcaFl40:
    Pds::EdtPdvCL::set_nbuffers(16);
    return new Pds::EdtPdvCL(*new Pds::OrcaCamera(info),id,channel);
  default:
    printf("Unsupported camera %s\n",Pds::DetInfo::name(info.device()));
    exit(1);
    break;
  }
  return NULL;
}

typedef std::list<Pds::Appliance*> AppList;

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
            unsigned              channel,
            const AppList&        user_apps,
            bool                  lCompress,
            bool                  lCopy,
            unsigned              evrmod,
            unsigned              evrchan,
            const char*           aliasName) :
      _task     (task),
      _platform (platform),
      _grabberId(grabberId),
      _channel  (channel),
      _user_apps(user_apps),
      _evrmod   (evrmod),
      _evrchan  (evrchan)
    {
      size_t max_size;
      const Pds::DetInfo info = static_cast<const Pds::DetInfo&>(src);
      switch(info.device()) {
      case DetInfo::Opal1000:
      case DetInfo::Opal2000:
      case DetInfo::Opal4000:
      case DetInfo::Opal1600:
      case DetInfo::Opal8000:
        max_size = Opal1k::max_row_pixels(info)*Opal1k::max_column_pixels(info)*2;
        _camman  = new Opal1kManager(src); break;
      case DetInfo::TM6740  :
        max_size = TM6740ConfigType::Row_Pixels*TM6740ConfigType::Column_Pixels*2;
        _camman  = new PimManager(src); break;
      case DetInfo::Quartz4A150:
        max_size = QuartzConfigType::Row_Pixels*QuartzConfigType::Column_Pixels*2;
        _camman  = new QuartzManager(src); break;
      case DetInfo::OrcaFl40:
        max_size = OrcaConfigType::Row_Pixels*OrcaConfigType::Column_Pixels*2;
        _camman = new OrcaManager(src); break;
      default:
        printf("Unsupported camera %s\n",DetInfo::name(info.device()));
        exit(1);
        break;
      }

      if (lCompress) {
        //
        //  Add extra buffer size for rare cases of compression enlargement
        //
        size_t msize = max_size*2;
        _user_apps.push_front(new FrameCompApp(msize));
        if (lCopy)
          msize += max_size;
        max_size = msize;
      }

      _sources.push_back(_camman->server().client());
      if (aliasName) {
        SrcAlias tmpAlias(_camman->server().client(), aliasName);
        _aliases.push_back(tmpAlias);
      }

      _max_size = max_size + 0x10000;
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
    const std::list<SrcAlias>* pAliases() const
    {
      return (_aliases.size() > 0) ? &_aliases : NULL;
    }
    bool     is_triggered() const { return true; }
    unsigned module      () const { return _evrmod; }
    unsigned channel     () const { return _evrchan; }
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
      _camman->attach(_driver(_grabberId, _channel, _sources.front()));
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

    unsigned max_event_size() const { return _max_size; }
  private:
    Task*          _task;
    unsigned       _platform;
    CameraManager* _camman;
    int            _grabberId;
    int            _channel;
    std::list<Src> _sources;
    std::list<SrcAlias> _aliases;
    AppList        _user_apps;
    unsigned       _max_size;
    unsigned       _evrmod;
    unsigned       _evrchan;
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const unsigned NO_PLATFORM = ~0;
  unsigned platform = NO_PLATFORM;
  bool lCompress = false;
  bool lCopy     = false;
  Arp* arp = 0;

  DetInfo info;
  AppList user_apps;

  unsigned grabberId(0);
  unsigned channel  (0);
  unsigned evrmod  (0);
  unsigned evrchan (0);
  bool infoFlag = false;
  bool helpFlag = false;

  extern char* optarg;
  extern int optind;
  char* uniqueid = (char *)NULL;
  int c;
  while ( (c=getopt( argc, argv, "a:i:p:g:c:L:C:tvu:h")) != EOF ) {
    switch(c) {
    case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
        printf("%s: option `-i' parsing error\n", argv[0]);
        helpFlag = true;
      } else {
        infoFlag = true;
      }
      break;
    case 'p':
      if (CmdLineTools::parseUInt(optarg,platform,evrmod,evrchan) != 3) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 'g':
      if (!CmdLineTools::parseUInt(optarg, grabberId)) {
        printf("%s: option `-g' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 'c':
      if (!CmdLineTools::parseUInt(optarg, channel)) {
        printf("%s: option `-c' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 'C':
      { unsigned ncopy;
        if (!CmdLineTools::parseUInt(optarg, ncopy)) {
          printf("%s: option `-C' parsing error\n", argv[0]);
          helpFlag = true;
        }
        lCompress = true;
        lCopy = ncopy!=0;
        FrameCompApp::setCopyPresample(ncopy);
      } break;
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
    case 'u':
      if (!CmdLineTools::parseSrcAlias(optarg)) {
        printf("%s: option `-u' parsing error\n", argv[0]);
        helpFlag = true;
      } else {
        uniqueid = optarg;
      }
      break;
    case 't':
      CameraBase::useTestPattern(true);
      break;
    case 'v':
      verbose = true;
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    case '?':
    default:
      helpFlag = true;
    }
  }

  if (!infoFlag) {
    printf("%s: detinfo is required\n", argv[0]);
    helpFlag = true;
  }

  if (platform == NO_PLATFORM) {
    printf("%s: platform is required\n", argv[0]);
    helpFlag = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    helpFlag = true;
  }

  if (helpFlag) {
    usage(argv[0]);
    return 1;
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

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,platform);
  info = DetInfo(node.pid(), info.detector(), info.detId(), info.device(), info.devId());

  printf("Compression is %s.\n", lCompress ? "enabled" : "disabled");

  SegTest* segtest = new SegTest(task, 
                                 platform, 
                                 info,
                                 grabberId,
                                 channel,
                                 user_apps,
                                 lCompress,
                                 lCopy,
                                 evrmod,
                                 evrchan,
                                 uniqueid);

  if (info.device()==DetInfo::Opal4000)
    ToEventWireScheduler::setMaximum(3);

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
