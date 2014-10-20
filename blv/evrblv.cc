#include "pdsapp/blv/IdleStream.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/OpenOutlet.hh"
#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvrManager.hh"
#include "pds/service/Task.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

using namespace Pds;

static void usage()
{
  printf("       -i [--info]        <det>/<detid>/<devid>\n");
  printf("       -r [--evrid]       <evr id>\n");
  printf("       -c [--controlport] <control port>\n");
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  char*     evrid     = 0;

  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);

  char* endPtr;

  unsigned controlPort = 1100;

  struct option loOptions[] = { { "info"       , 1, 0, 'i'},
        { "evrid"      , 1, 0, 'r'},
        { "controlport", 1, 0, 'c'} };

  int c;
  while ( (c=getopt_long( argc, argv, "i:p:r:c:D:", loOptions, NULL)) != EOF ) {
    switch(c) {
    case 'i':
      det    = (DetInfo::Detector)strtoul(optarg, &endPtr, 0); endPtr++;
      if ( *endPtr == 0 ) break;
      detid  = strtoul(endPtr, &endPtr, 0); endPtr++;
      if ( *endPtr == 0 ) break;
      devid  = strtoul(endPtr, &endPtr, 0);
      break;
    case 'r':
      evrid = optarg;
      break;
    case 'c':
      controlPort = strtoul(optarg, NULL, 0);
      break;
    default:
      usage();
      return 1;
    }
  }

  char defaultdev='a';
  if (!evrid) evrid = &defaultdev;

  char evrdev[16];
  sprintf(evrdev,"/dev/er%c3",*evrid);
  printf("Using evr %s\n",evrdev);

  Node node(Level::Source, 0);
  printf("Using src %x/%x/%x/%x\n",det,detid,DetInfo::Evr,devid);
  DetInfo detInfo(node.pid(),det,detid,DetInfo::Evr,devid);

  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  CfgClientNfs* cfgService = new CfgClientNfs(detInfo);

  //
  //  Setup the idle stream
  //
  ProcInfo idleSrc(Level::Segment,0,0);
  IdleStream*   idleStream = new IdleStream(controlPort, idleSrc);
  new OpenOutlet(*idleStream->outlet());
  //  attach the server
  EvrManager& ievrmgr = *new EvrManager(erInfo, *cfgService, true, 0 /* evr module 0*/);
  ievrmgr.appliance().connect(idleStream->inlet());
  idleStream->start();

  Task* task = new Task(Task::MakeThisATask);
  task->mainLoop();

  return 0;
}
