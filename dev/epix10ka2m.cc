/*-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : epix10ka2m.cc
-- Author     : Matt Weaver <weaver@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2018-09-28
-- Last update: 2018-09-17
-- Platform   : 
-- Standard   : C++
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- This file is part of 'LCLS DAQ'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'LCLS DAQ', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------*/

#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/service/Task.hh"
#include "pds/epicstools/EventcodeQuery.hh"
#include "pds/epix10ka2m/Manager.hh"
#include "pds/epix10ka2m/Server.hh"
#include "pds/epix10ka2m/ServerSim.hh"
#include "pds/epix10ka2m/Configurator.hh"
#include "pds/pgp/Pgp.hh"

#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <new>

using namespace Pds;

static std::vector<Pds::Epix10ka2m::Server*> serverList(4);

static bool cleanupTriggers = false;

void sigHandler( int signal ) {
  psignal( signal, "Signal received by Epix10ka2M Application");
  for(unsigned i=0; i<serverList.size(); i++) {
    Pds::Epix10ka2m::Server* server = serverList[i];
    if (server != 0) {
      if (server->configurator() && cleanupTriggers) {
        server->configurator()->waitForFiducialMode(false);
        server->configurator()->evrLaneEnable(false);
        //      server->configurator()->enableExternalTrigger(false);
        server->configurator()->cleanupEvr(1);
      }
      server->disable();
      //      server->dumpFrontEnd();
      server->die();
    } else {
      printf("sigHandler found nil server 1!\n");
    }
  }
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}

void printUsage(char* s) {
  printf( "Usage: epix10ka [options]\n"
      "    -h      Show usage\n"
      "    -d      Set detector type by name [Default: XcsEndstation]\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -P      Set pgpcard and port number  [Default: 0]\n"
      "                The format of the index number is a one byte number with the bottom nybble being\n"
      "                the index of the card and the top nybble being index of the port in use with the"
      "                first port being ONE.  For the G3 card this could be a number from 1 to 8, for the\n"
      "                G2 card, this could be a number from 1 to 4.\n"
      "    -E      Clean up trigger settings on exit when using triggering over fiber\n"
      "                Note: this should not be used if there is an IOC monitoring the detector\n"
      "                environmental data. The IOC needs the triggers running to get data\n"
      "    -e <N>  Set the maximum event depth, default is 128\n"
      "    -R <B>  Set flag to reset on every config or just the first if false\n"
      "    -m <B>  Set flag to maintain or not maintain lost run triggers (turn off for slow running\n"
      "    -r <Q>  Replicate Q quads\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offest and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label Epix10kaL1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label Epix10kaServer enable and disable\n"
      "                bit 08          turn on printing of FE Internal status on\n"
      "                "
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::NoDetector;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  unsigned            pgpcard[4]          = { 0x10, 0x20, 0x30, 0x40 };
  unsigned            eventDepth          = 64;
  bool                maintainRunTrig     = false;
  unsigned            nthreads            = 4;
  unsigned            replicateQuads      = 0;
  ::signal( SIGINT,  sigHandler );
  ::signal( SIGSEGV, sigHandler );
  ::signal( SIGFPE,  sigHandler );
  ::signal( SIGTERM, sigHandler );
  ::signal( SIGQUIT, sigHandler );

  extern char* optarg;
  char* endptr;
  char* uniqueid = (char *)NULL;
  int c;
  while( ( c = getopt( argc, argv, "hd:i:p:m:e:R:D:H:P:r:u:E" ) ) != EOF ) {
    printf("processing %c\n", c);
    bool     found;
    unsigned index;
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
    case 'p':
      if (CmdLineTools::parseUInt(optarg,platform)!=1) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        return 0; 
      }
      break;
    case 'i':
      deviceId = strtoul(optarg, NULL, 0);
      break;
    case 'P':
      { unsigned q=0;
        pgpcard[q++] = strtoul(optarg, &endptr, 0);
        while(*endptr==',' && q<4)
          pgpcard[q++] = strtoul(endptr+1, &endptr, 0);
        printf("epix10ka read pgpcard as [");
        for(unsigned i=0; i<q; i++)
          printf(" 0x%x", pgpcard[i]);
        printf("]\n"); }
      break;
    case 'e':
      eventDepth = strtoul(optarg, NULL, 0);
      printf("Epix10ka using event depth of  %u\n", eventDepth);
      break;
    case 'H':
      nthreads = strtoul(optarg, NULL, 0);
      break;
    case 'R':
      Pds::Epix10ka2m::Server::resetOnEveryConfig(strtoul(optarg, NULL, 0));
      break;
    case 'm':
      maintainRunTrig = strtoul(optarg, NULL, 0);
      printf("Setting maintain run trigger to %s\n", maintainRunTrig ? "true" : "false");
      break;
    case 'D':
      Pds::Epix10ka2m::Server::debug(strtoul(optarg, NULL, 0));
      break;
    case 'r':
      replicateQuads = strtoul(optarg, NULL, 0);
      break;
    case 'E':
      cleanupTriggers = true;
      break;
    case 'h':
      printUsage(argv[0]);
      return 0;
      break;
    case 'u':
      if (!CmdLineTools::parseSrcAlias(optarg)) {
        printf("%s: option `-u' parsing error\n", argv[0]);
      } else {
        uniqueid = optarg;
      }
      break;
    default:
      printf("Error: Option could not be parsed!\n");
      printUsage(argv[0]);
      return 0;
      break;
    }
  }

  Pds::Node node(Level::Source, platform);
  DetInfo detInfo(node.pid(), detector, 0, DetInfo::Epix10ka2M, deviceId);

  printf("Epix10ka2M will reset on %s configuration\n", Pds::Epix10ka2m::Server::resetOnEveryConfig() ? "every" : "only the first");

  char devName[128];
  char err[128];

  //  Pds::Pgp::Pgp::portOffset(lane);

  std::list<Pds::EbServer*> ebServerList;
  for(unsigned s=0; s<serverList.size(); s++) {

    Pds::Epix10ka2m::ServerSequence* server = new Pds::Epix10ka2m::ServerSequence(detInfo, 0);

    unsigned card = pgpcard[s] & 0xf;
    unsigned port = (pgpcard[s] >> 4) & 0xf;
    unsigned lane = port - 1;

    printf("arg 0x%x : card %u  port %u lane %u\n",
           pgpcard[s], card, port, lane);

    sprintf(devName, "/dev/pgpcard_%u", card);

    int fd = open( devName,  O_RDWR | O_NONBLOCK );
    if (fd < 0) {
      sprintf(err, "%s opening %s failed", argv[0], devName);
      perror(err);
      return -1;
    }

    // Check that the driver is using a compatible API version
    if (!Pgp::Pgp::checkVersion(true, fd)) {
      fprintf(stderr, "Driver reports an incompatible DMA API version!\n");
      return 1;
    }

    //  Allocate for VC 0(Data), 2(Scope)
    { Pgp::Pgp p(true, fd);
      p.allocateVC(5,1<<lane); }

    //  Open a second time for an independent stream (register configuration)
    int fd2 = open( devName,  O_RDWR | O_NONBLOCK );
    //  Allocate for VC 1(Registers)
    { Pgp::Pgp p(true, fd2);
      p.allocateVC(2,1<<lane); }

    printf("%s pgpcard opened as fd %d,%d lane %d\n", argv[0], fd, fd2, lane);

    server->setFd(fd, fd2, lane, s);

    serverList  [s] = server;
    ebServerList.push_back(server);

    if (replicateQuads && (replicateQuads+s)==3) break;
  }

  if (replicateQuads) {
    serverList.resize(4);
    Pds::Epix10ka2m::ServerSim* sim;
    unsigned i=0;
    while(i<4-replicateQuads) {
      ebServerList.push_back(sim=new Pds::Epix10ka2m::ServerSim(serverList[i]));
      serverList[i] = sim;
      i++;
    }
    while(i<4) {
      ebServerList.push_back(sim=new Pds::Epix10ka2m::ServerSim(sim));
      serverList[i] = sim;
      i++;
    }
    for(i=0; i<4-replicateQuads; i++)
      ebServerList.pop_front();
  }

  Pds::Epix10ka2m::Manager* manager = new Pds::Epix10ka2m::Manager(serverList, nthreads);

  //  EPICS thread initialization
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ),
           "epix10ka calling ca_context_create" );
  EventcodeQuery::execute();

  Task* task = new Task( Task::MakeThisATask );
  std::list<Appliance*> apps;

  StdSegWire settings(ebServerList, uniqueid, 5<<20, eventDepth, 
                      false, 0, 0, true, true);
  EventAppCallback* seg = new EventAppCallback(task, platform, manager->appliance());
   
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();

  ca_context_destroy();

  return 0;
}
