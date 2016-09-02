#include "pdsdata/xtc/DetInfo.hh"

#include "pdsapp/tools/EventOptions.hh"
#include "pdsapp/tools/RecorderQ.hh"
#include "pdsapp/tools/CountAction.hh"
#include "pdsapp/tools/StatsTree.hh"
#include "pds/client/Decoder.hh"

#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/FastSegWire.hh"
#include "pds/service/Task.hh"
#include "pds/tprds/Server.hh"
#include "pds/tprds/Manager.hh"
#include "pds/tprds/Module.hh"

#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <poll.h>

extern int optind;

using namespace Pds;

static void usage(const char *p)
{
  printf("Usage: %s -i <detid> -p <platform> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "\t -i <detid>             detector ID (e.g. 0/0/0)\n"
         "\t -p <platform>          platform number\n"
         "\t -r <evrid>             evr ID (e.g., a, b, c, or d) (default: a)\n"
         "\t -u <alias>             set device alias\n"
         "\t -m                     enable monitor channel\n"
         "\t -h                     print this message and exit\n", p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const uint32_t NO_PLATFORM = uint32_t(-1UL);
  uint32_t  platform  = NO_PLATFORM;
  char*     evrid     = 0;
  bool      lUsage    = false;
  bool      lMonitor  = false;

  Pds::DetInfo info(0,DetInfo::NoDetector,0,DetInfo::NoDevice,0);

  char* uniqueid = (char *)NULL;
  EventOptions options;

  int c;
  while ( (c=getopt( argc, argv, "i:p:r:u:def:mh")) != EOF) {
    switch(c) {
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
        printf("%s: option `-i' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'p':
      if (!CmdLineTools::parseUInt(optarg,platform)) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'r':
      evrid = optarg;
      if (strlen(evrid) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'u':
      if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
        printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
      } else {
        uniqueid = optarg;
      }
      break;
    case 'd': options.mode = EventOptions::Display; break;
    case 'e': options.mode = EventOptions::Decoder; break;
    case 'f': options.outfile = optarg; break;
    case 'm': lMonitor = true; break;
    case 'h':
      usage(argv[0]);
      return 0;
    case '?':
    default:
      lUsage = true;
    }
  }

  if (platform==NO_PLATFORM) {
    printf("%s: platform required\n",argv[0]);
    lUsage = true;
  }

  options.platform = platform;

//   if (optind < argc) {
//     printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
//     lUsage = true;
//   }

  if (info.detector() == Pds::DetInfo::NumDetector) {
    printf("%s: detinfo is required\n", argv[0]);
    lUsage = true;
  }

  if (!options.validate(argv[0]))
    return 0;

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  { char evrdev[16];
    sprintf(evrdev,"/dev/er%c3",evrid ? evrid[0] : 'a');
    printf("Using evr %s\n",evrdev);

    int fd = open(evrdev, O_RDWR);
    if (fd<0) {
      perror("Could not open");
      return -1;
    }

    void* ptr = mmap(0, sizeof(Tpr::EvrReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      perror("Failed to map");
      return -2;
    }

    Tpr::EvrReg* p = reinterpret_cast<Tpr::EvrReg*>(ptr);

    printf("SLAC Version[%p]: %08x\n", 
	   &(p->evr),
	   ((volatile uint32_t*)(&p->evr))[0x30>>2]);

    p->evr.IrqEnable(0);

    printf("Axi Version [%p]: BuildStamp: %s\n", 
	   &(p->version),
	   p->version.buildStamp().c_str());
    
    printf("[%p] [%p] [%p]\n",p, &(p->version), &(p->xbar));

    p->xbar.setTpr(Pds::Tpr::XBar::StraightIn);
    p->xbar.setTpr(Pds::Tpr::XBar::StraightOut);
  }

  int fd = -1;
  { char evrdev[16];
    sprintf(evrdev,"/dev/er%c3_1",evrid ? evrid[0] : 'a');
    printf("Using tpr %s\n",evrdev);

    fd = open(evrdev, O_RDWR);
    if (fd<0) {
      perror("Could not open");
      return -1;
    }
  }

  void* ptr = mmap(0, sizeof(TprDS::TprReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Failed to map");
    exit(1);
  }

  TprDS::TprReg* p = reinterpret_cast<TprDS::TprReg*>(ptr);
  
  { for(unsigned i=0; i<12; i++)
      p->base.channel[i].control=0;
    uint32_t data[1024];
    Tpr::RxDesc desc(data,1024);
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    while( ::poll(&pfd,1,1000)>0 )
      ::read(fd,&desc,sizeof(Tpr::RxDesc));
  }
  

  Pds::DetInfo src(p->base.partitionAddr,
                   info.detector(),info.detId(),
                   info.device  (),info.devId());

  Node node(Level::Segment, platform);
  printf("Using %s\n",Pds::DetInfo::name(src));

  TprDS::Server*  server  = new TprDS::Server (fd, src);
  TprDS::Manager* manager = new TprDS::Manager(*p, *server, 
                                               *new CfgClientNfs(src),
                                               lMonitor);

  Task* task = new Task(Task::MakeThisATask);
  std::list<Appliance*> apps;
  if (options.outfile)
    apps.push_back(new RecorderQ(options.outfile, 
                                 options.chunkSize, 
                                 options.uSizeThreshold, 
                                 options.delayXfer, 
                                 NULL,
                                 options.expname));
  switch(options.mode) {
  case EventOptions::Counter:
    apps.push_back(new CountAction); break;
  case EventOptions::Decoder:
    apps.push_back(new Decoder(Level::Segment)); break;
  case EventOptions::Display:
    apps.push_back(new StatsTree); break;
  default:
    break;
  }
  apps.push_back(&manager->appliance());
  EventAppCallback* seg = new EventAppCallback(task, platform, apps);
  FastSegWire settings(*server, p->base.partitionAddr, uniqueid, 
                       (1<<12), 16, (1<<12));
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg);
  seglevel->attach();

  task->mainLoop();
  return 0;
}
