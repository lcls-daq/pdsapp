#include "pdsdata/xtc/DetInfo.hh"

#include "pds/config/CfgClientNfs.hh"
#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/FastSegWire.hh"
#include "pds/service/Task.hh"
#include "pds/xpm/Server.hh"
#include "pds/xpm/Manager.hh"
#include "pds/xpm/Module.hh"

#include "cadef.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

extern int optind;

using namespace Pds;

static void sigHandler( int signal ) {
  Xpm::Module* m = new(0) Xpm::Module;
  m->setL0Enabled(false);
  ::exit(signal);
}

static void usage(const char *p)
{
  printf("Usage: %s -p <platform> -a <xpm ip address> [-u <alias>]\n"
         "\n"
         "Options:\n"
         "\t -p <platform>          platform number\n"
         "\t -a <ip addr>           xpm private ip address (dotted notation)\n"
         "\t -u <alias>             set device alias\n"
         "\t -h                     print this message and exit\n", p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const uint32_t NO_PLATFORM = uint32_t(-1UL);
  uint32_t  platform  = NO_PLATFORM;
  const char* xpm_ip  = 0;
  bool      lUsage    = false;
  int fixedRate=-1;

  Pds::DetInfo info(getpid(),DetInfo::NoDetector,0,DetInfo::Evr,0);

  char* uniqueid = (char *)NULL;

  int c;
  while ( (c=getopt( argc, argv, "a:p:u:F:h")) != EOF ) {
    switch(c) {
    case 'a':
      xpm_ip = optarg;
      break;
    case 'p':
      if (!CmdLineTools::parseUInt(optarg,platform)) {
        printf("%s: option `-p' parsing error\n", argv[0]);
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
    case 'F':
      fixedRate = atoi(optarg);
      break;
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

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  Node node(Level::Segment, platform);
  printf("Using %s\n",Pds::DetInfo::name(info));

  Pds::Cphw::Reg::set(xpm_ip, 8192, 0);
  Pds::Xpm::Module* m = new((void*)0x80000000) Pds::Xpm::Module;
  m->setL0Enabled(false);
  if (fixedRate>=0)
    m->setL0Select_FixedRate(fixedRate);

  Xpm::Server*  server  = new Xpm::Server (*m, info);
  Xpm::Manager* manager = new Xpm::Manager(*m, *server,
                                           *new CfgClientNfs(info));

//   //  EPICS thread initialization
//   SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), 
//            "control calling ca_context_create" );

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, manager->appliance());
  FastSegWire settings(*server, -1, uniqueid, 1024);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg);
  seglevel->attach();

  ::signal( SIGINT, sigHandler );

  task->mainLoop();

  //  ca_context_destroy();

  return 0;
}
