// $Id$

#include "pdsdata/xtc/DetInfo.hh"

#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/rayonix/RayonixManager.hh"
#include "pds/rayonix/RayonixServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <climits>

extern int optind;

static void usage(const char *p)
{
  printf("Usage: %s -i <detid> -p <platform>,<mod>,<chan> [OPTIONS]\n", p);
  printf("\n"
         "Options:\n"
         "    -i <detid>                  detector ID (e.g. 22 for XppEndstation)\n"
         "    -p <platform>,<mod>,<chan>  platform number, EVR module, EVR channel\n"
         "    -u <alias>                  set device alias\n"
         "    -v                          increase verbosity (may be repeated)\n"
         "    -h                          print this message and exit\n");
}

using namespace Pds;

static const unsigned MAX_EVENT_SIZE = 128*1024*1024;
static const unsigned MAX_EVENT_DEPTH = 32;

int main( int argc, char** argv )
{
  unsigned detid = UINT_MAX;
  unsigned platform = UINT_MAX;
  unsigned module = 0;
  unsigned channel = 0;
  unsigned verbosity = 0;
  bool helpFlag = false;
  char* uniqueid = (char *)NULL;

  extern char* optarg;
  int c;
  while( ( c = getopt( argc, argv, "i:p:vhu:" ) ) != EOF ) {
    switch(c) {
      case 'i':
        if (!CmdLineTools::parseUInt(optarg,detid)) {
          printf("%s: option `-i' parsing error\n", argv[0]);
          helpFlag = true;
        }
        break;
      case 'u':
        if (!CmdLineTools::parseSrcAlias(optarg)) {
          printf("%s: option `-u' parsing error\n", argv[0]);
          helpFlag = true;
        } else {
          uniqueid = optarg;
        }
        break;
      case 'h':
        usage(argv[0]);
        return 0;
      case 'v':
        ++verbosity;
        break;
      case 'p':
        if (CmdLineTools::parseUInt(optarg,platform,module,channel) != 3) {
          printf("%s: option `-p' parsing error\n", argv[0]);
          helpFlag = true;
        }
        break;
      case '?':
      default:
        helpFlag = true;
        break;
      }
   }

   if (platform == UINT_MAX) {
      printf("%s: platform is required\n", argv[0]);
      helpFlag = true;
   }

   if (detid == UINT_MAX) {
      printf("%s: detid is required\n", argv[0]);
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

  Node node( Level::Source, platform );

  Task* task = new Task( Task::MakeThisATask );

  CfgClientNfs* cfgService;
  std::list<EbServer*> servers;
  std::list<RayonixManager*> managers;

  DetInfo detInfo( node.pid(),
                   (Pds::DetInfo::Detector) detid,
                   0,
                   DetInfo::Rayonix,
                   0 );

  cfgService = new CfgClientNfs(detInfo);

  RayonixServer* rayonixServer = new RayonixServer(detInfo, verbosity);
  servers.push_back(rayonixServer);
  RayonixManager* rayonixMgr = new RayonixManager(rayonixServer, cfgService);
  managers.push_back(rayonixMgr);

  StdSegWire settings(servers, uniqueid, MAX_EVENT_SIZE, MAX_EVENT_DEPTH, true, module, channel);
  EventAppCallback* seg = new EventAppCallback(task, platform, managers.front()->appliance());
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);

  if (seglevel->attach()) {
    printf("entering rayonix task main loop\n");
    task->mainLoop();
    printf("exiting rayonix task main loop\n");
  }

   return 0;
}
