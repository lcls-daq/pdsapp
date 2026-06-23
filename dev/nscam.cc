#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/nscam/Manager.hh"
#include "pds/nscam/Server.hh"
#include "pds/nscam/Detector.hh"
#include "pds/nscam/Error.hh"
#define _NOLOGGERMACROS 1
#include "pds/nscam/Logger.hh"
#include "pds/config/CfgClientNfs.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <list>
#include <vector>

static const unsigned EVENT_SIZE_EXTRA = 0x10000;
static const unsigned MAX_FRAME_SIZE = 1024*512*2;
static const unsigned MAX_EVENT_DEPTH = 32;

using namespace Pds;

static const NsCam::Logger::Level logLevels[] = {
  NsCam::Logger::Level::ERROR,
  NsCam::Logger::Level::WARN,
  NsCam::Logger::Level::INFO,
  NsCam::Logger::Level::DEBUG,
};
static const size_t numLogLevels = sizeof(logLevels) / sizeof(logLevels[0]);

static void nscamUsage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "    -i|--id       <detinfo>                 int/int/int/int or string/int/string/int\n"
         "                                            (e.g. XppEndStation/0/Uxi/1 or 22/0/47/1)\n"
         "    -p|--platform <platform>,<mod>,<chan>   platform number, EVR module, EVR channel\n"
         "    -u|--uniqueid <alias>                   set device alias\n"
         "    -P|--port     <port>                    set the UXI detector port (default: 20482)\n"
         "    -H|--host     <host>                    set the UXI detector host/ip\n"
         "    -m|--max      <max_frames>              set the maximum number of frames to expect per event (default: 8)"
         "    -d|--debug    <level>                   Set debug level (default: 1)\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hp:i:u:P:H:m:d:";
  const struct option loOptions[]   =
  {
    {"help",        0, 0, 'h'},
    {"platform",    1, 0, 'p'},
    {"id",          1, 0, 'i'},
    {"uniqueid",    1, 0, 'u'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"max",         1, 0, 'm'},
    {"debug",       1, 0, 'd'},
    {0,             0, 0,  0 }
  };

  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned port = 20482;
  unsigned max_num_frames = 8;
  unsigned debug = 1;
  bool lUsage = false;
  bool isTriggered = false;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NumDetector, 0, DetInfo::Uxi, 0);
  char* uniqueid = (char *)NULL;
  std::string hostname = "";

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        nscamUsage(argv[0]);
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
        if (!Pds::CmdLineTools::parseUInt(optarg,port)) {
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
      case 'd':
        if (!Pds::CmdLineTools::parseUInt(optarg,debug)) {
          printf("%s: option `-d' parsing error\n", argv[0]);
          lUsage = true;
        } else if (debug >= numLogLevels) {
          printf("%s: option `-d' value out of range\n", argv[0]);
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
    nscamUsage(argv[0]);
    return 1;
  }

  try {
    // initialize nscam logger
    NsCam::Logger::instance().setLevel(logLevels[debug]);

    // create the detector class - don't init the detector here defer to first config
    printf("Attempting to connect to the detector at %s:%u\n", hostname.c_str(), port);
    auto start = std::chrono::steady_clock::now();
    NsCam::Detector det(hostname, port, NsCam::CommType::GIGE, NsCam::BoardType::LLNL_V1, NsCam::SensorType::ICARUS2, false);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    printf("Connected to the detector in %f seconds\n\n", elapsed_seconds.count());

    CfgClientNfs* cfg = new CfgClientNfs(detInfo);

    std::list<EbServer*>     servers;
    std::list<NsCam::Manager*> managers;

    NsCam::Server* srv = new NsCam::Server(detInfo);
    servers   .push_back(srv);
    NsCam::Manager* mgr = new NsCam::Manager(det, *srv, *cfg, max_num_frames);
    managers.push_back(mgr);

    StdSegWire settings(servers, uniqueid, MAX_FRAME_SIZE*max_num_frames + EVENT_SIZE_EXTRA, MAX_EVENT_DEPTH, isTriggered, module, channel);

    Task* task = new Task(Task::MakeThisATask);
    EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
    SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
    if (seglevel->attach()) {
      task->mainLoop();
    }
  } catch(const NsCam::NsCamException& err) {
    printf("Aborting: Failed to connect to the detector (%s:%u): %s\n", hostname.c_str(), port, err.what());
    return 1;
  }

  return 0;
}
