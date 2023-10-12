#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/uxi/Manager.hh"
#include "pds/uxi/Server.hh"
#include "pds/uxi/Detector.hh"
#include "pds/config/CfgClientNfs.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <list>
#include <vector>

static const unsigned EVENT_SIZE_EXTRA = 0x10000;
static const unsigned MAX_FRAME_SIZE = 1024*512*2;
static const unsigned MAX_EVENT_DEPTH = 32;


static Pds::Uxi::Detector* det = NULL;

using namespace Pds;

static void shutdown(int signal)
{
  if (det) {
    det->disconnect();
  }
  exit(signal);
}

static void uxiUsage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "    -i|--id       <detinfo>                 int/int/int/int or string/int/string/int\n"
         "                                            (e.g. XppEndStation/0/Uxi/1 or 22/0/47/1)\n"
         "    -p|--platform <platform>,<mod>,<chan>   platform number, EVR module, EVR channel\n"
         "    -u|--uniqueid <alias>                   set device alias\n"
         "    -P|--port     <portset>                 set the UXI detector server port set (default: 0)\n"
         "    -H|--host     <host>                    set the UXI detector server host ip (default: localhost)\n"
         "    -m|--max      <max_frames>              set the maximum number of frames to expect per event (default: 8)"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:P:H:m:";
  const struct option loOptions[]   =
  {
    {"help",        0, 0, 'h'},
    {"platform",    1, 0, 'p'},
    {"id",          1, 0, 'i'},
    {"uniqueid",    1, 0, 'u'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"max",         1, 0, 'm'},
    {0,             0, 0,  0 }
  };

  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned portset = 0;
  unsigned baseport = Pds::Uxi::Detector::BasePort;
  unsigned max_num_frames = 8;
  bool lUsage = false;
  bool isTriggered = false;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::Uxi, 0);
  char* uniqueid = (char *)NULL;
  const char* default_host = "localhost";
  char* hostname = (char *)NULL;

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        uxiUsage(argv[0]);
        return 0;
      case 'p':
        switch (CmdLineTools::parseUInt(optarg,platform,module,channel)) {
          case 1:
            isTriggered = false;
            break;
          case 3:
            isTriggered = true;
            break;
          default:
            printf("%s: option `-p' parsing error\n", argv[0]);
            lUsage = true;
            break;
        }
        break;
      case 'i':
        if (!CmdLineTools::parseDetInfo(optarg,detInfo)) {
          printf("%s: option `-i' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'u':
        if (!CmdLineTools::parseSrcAlias(optarg)) {
          printf("%s: option `-u' parsing error\n", argv[0]);
          lUsage = true;
        } else {
          uniqueid = optarg;
        }
        break;
      case 'H':
        hostname = optarg;
        break;
      case 'P':
        if (!Pds::CmdLineTools::parseUInt(optarg,portset)) {
          printf("%s: option `-p' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'm':
        if (!Pds::CmdLineTools::parseUInt(optarg,max_num_frames)) {
          printf("%s: option `-m' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    uxiUsage(argv[0]);
    return 1;
  }

  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = shutdown;
  sigActionSettings.sa_flags   = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGINT\n");
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGTERM\n");

  std::list<EbServer*>     servers;
  std::list<Uxi::Manager*> managers;

  CfgClientNfs* cfg = new CfgClientNfs(detInfo);

  det = new Uxi::Detector(
    hostname ? hostname : default_host,
    baseport + 2 * portset,
    baseport + 2 * portset +1
  );

  if (!det->connected()) {
    printf("Aborting: Failed to connect to the Uxi detector, please check that the local uxi_server process is running!\n");
    delete det;
    det = 0;
    return 1;
  }

  Uxi::Server* srv = new Uxi::Server(detInfo);
  servers   .push_back(srv);
  Uxi::Manager* mgr = new Uxi::Manager(*det, *srv, *cfg, max_num_frames);
  managers.push_back(mgr);

  StdSegWire settings(servers, uniqueid, MAX_FRAME_SIZE*max_num_frames + EVENT_SIZE_EXTRA, MAX_EVENT_DEPTH, isTriggered, module, channel);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  if (seglevel->attach()) {
    task->mainLoop();
  }

  return 0;
}
