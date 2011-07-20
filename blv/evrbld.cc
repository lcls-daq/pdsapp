#include "pdsapp/blv/IdleStream.hh"
#include "pdsapp/blv/ToBldEventWire.hh"
#include "pdsapp/blv/EvrBldManager.hh"
#include "pdsapp/blv/PipeApp.hh"
#include "pdsapp/config/Path.hh"
#include "pdsapp/config/Experiment.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/utility/OpenOutlet.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <list>

static EvrBldManager* evrBldMgr = NULL; 

static int openFifo(const char* name, int flags, unsigned index) {
  char path[128];
  sprintf(path,"/tmp/pimbld_%s_%d",name,index);
  int result = ::open(path,flags);
  return result;
}

//
//  Need to stop triggers on exit
//
static void signalHandler(int sigNo)
{
  printf( "\n signalHandler(): signal %d received.\n",sigNo ); 
  if (evrBldMgr )   {
    evrBldMgr->stop();  
    evrBldMgr->disable();
  }
  exit(0); 
}

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -r <evr a/b> -d <configdb path> -p <control port> -c <det,det_id,device_id> -n <number of clients>\n",p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const char* evrid(0);
  const char* dbpath(0);
  unsigned controlPort=1100;
  unsigned clients=1;
  unsigned detector(0), detector_id(0), device_id(0);

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "r:p:c:d:n:")) != EOF ) {
    switch(c) {
    case 'r':
      evrid = optarg;
      break;
    case 'p':
      controlPort = strtoul(optarg, NULL, 0);
      break;
    case 'c':
      detector    = strtoul(optarg, &endPtr, 0);
      detector_id = strtoul(endPtr+1, &endPtr, 0);
      device_id   = strtoul(endPtr+1, &endPtr, 0);
      break;
    case 'd':
      dbpath = optarg;
      break;
    case 'n':
      clients = strtoul(optarg, NULL, 0);
      break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  if (evrid==0 || controlPort==0) {
    usage(argv[0]);
    exit(1);
  }

  int cntl_fd  = openFifo("TransInputQ",O_WRONLY,0);
  int read_fd  = openFifo("TransInputQ",O_RDONLY,clients);

  std::list<int> evr_write_fd;
  for(unsigned i=0; i<clients; i++)
    evr_write_fd.push_back( openFifo("EventInputQ",O_WRONLY,i) );

#if 1
  timespec ts;
  ts.tv_sec = 2;
  ts.tv_nsec = 0;
  nanosleep(&ts,0);
#endif

  Task* task = new Task(Task::MakeThisATask);

  Node node(Level::Source,0);

  //
  //  Setup the idle stream
  //
  ProcInfo idleSrc(Level::Segment,0,0);
  unsigned key(0);
  if (dbpath) {
    Pds_ConfigDb::Path path(dbpath);
    Pds_ConfigDb::Experiment expt(path);
    expt.read();
    key = strtoul(expt.table().get_top_entry("BLD")->key().c_str(),NULL,16);
  }
  IdleStream* idle = new IdleStream(controlPort, 
				    idleSrc,
                                    dbpath,
                                    key);

  (new PipeApp(read_fd, cntl_fd))->connect(idle->inlet());

  //  create the inlet wire/event builder
  const int MaxSize = 0x100000;
  const int ebdepth = 4;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *idle->inlet(),
					*new OpenOutlet(*idle->outlet()), 0, 0x7f000001, MaxSize, ebdepth, 0);
  iwire->connect();

  DetInfo det(node.pid(), 
              (DetInfo::Detector)detector, detector_id,
              DetInfo::TM6740, device_id);
  evrBldMgr = new EvrBldManager(det,evrid,evr_write_fd);
  evrBldMgr->appliance().connect(idle->inlet());

  idle->set_inlet_wire(iwire);
  idle->start();

  //
  // Register signal handler to exit gracefully
  //
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = signalHandler; 
  sigActionSettings.sa_flags = 0;
  sigActionSettings.sa_flags   |= SA_RESTART;    
  
  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 ) 
    printf( "main(): Cannot register signal handler for SIGINT\n" );
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 ) 
    printf( "main(): Cannot register signal handler for SIGTERM\n" );

  printf("::start()\n");

  task->mainLoop();

  return 0;
}
