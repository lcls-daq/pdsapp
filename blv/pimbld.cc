#include "pdsapp/blv/IdleStream.hh"
#include "pdsapp/blv/ToBldEventWire.hh"
#include "pdsapp/blv/EvrBldServer.hh"
#include "pdsapp/blv/EvrBldManager.hh"
#include "pdsapp/config/Path.hh"
#include "pdsapp/config/Experiment.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/camera/PimManager.hh"
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

#include <list>

class CamParams {
public:
  unsigned bld_id;
  unsigned detector;
  unsigned detector_id;
  unsigned device_id;
  unsigned grabber_id;
};

static void *thread_signals(void*)
{
  while(1) sleep(100);
  return 0;
}

static EvrBldManager* evrBldMgr = NULL; 

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
  printf("Usage: %s -r <evr a/b> -i <multicast interface> -d <configdb path> -p <control port> -c <cam parameters> [-d ...]\n",p);
  printf("\t\"cam parameters\" : bld_id, detector, detector_id, device_id, grabber_id\n");
}

static unsigned parse_network(const char* arg)
{
  unsigned interface = 0;
  if (arg[0]<'0' || arg[0]>'9') {
    int skt = socket(AF_INET, SOCK_DGRAM, 0);
    if (skt<0) {
      perror("Failed to open socket\n");
      exit(1);
    }
    ifreq ifr;
    strcpy( ifr.ifr_name, optarg);
    if (ioctl( skt, SIOCGIFADDR, (char*)&ifr)==0)
      interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
    else {
      printf("Cannot get IP address for network interface %s.\n",optarg);
    }
    printf("Using interface %s (%d.%d.%d.%d)\n",
           arg,
           (interface>>24)&0xff,
           (interface>>16)&0xff,
           (interface>> 8)&0xff,
           (interface>> 0)&0xff);
    close(skt);
  }
  else {
    in_addr inp;
    if (inet_aton(arg, &inp))
      interface = ntohl(inp.s_addr);
  }
  return interface;
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned interface(0);
  const char* evrid(0);
  const char* dbpath(0);
  unsigned controlPort=1100;
  std::list<CamParams> cam_list;

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "i:r:c:p:d:")) != EOF ) {
    switch(c) {
    case 'i':
      interface = parse_network(optarg);
      break;
    case 'r':
      evrid = optarg;
      break;
    case 'p':
      controlPort = strtoul(optarg, NULL, 0);
      break;
    case 'd':
      dbpath = optarg;
      break;
    case 'c':
      CamParams cam;
      cam.bld_id      = strtoul(optarg  ,&endPtr,0);
      cam.detector    = strtoul(endPtr+1,&endPtr,0);
      cam.detector_id = strtoul(endPtr+1,&endPtr,0);
      cam.device_id   = strtoul(endPtr+1,&endPtr,0);
      cam.grabber_id  = strtoul(endPtr+1,&endPtr,0);
      cam_list.push_back(cam);
      break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  if (evrid==0 || interface==0 || controlPort==0) {
    usage(argv[0]);
    exit(1);
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

  std::map<unsigned,BldInfo> bldmap;
  for(std::list<CamParams>::iterator it=cam_list.begin(); it!=cam_list.end(); it++) {
    DetInfo det(node.pid(), 
                (DetInfo::Detector)it->detector, it->detector_id,
                DetInfo::TM6740, it->device_id);
    BldInfo bld(node.pid(),
                (BldInfo::Type)it->bld_id);
    bldmap[det.phy()] = bld;
  } 

  ToBldEventWire* outlet = new ToBldEventWire(*idle->outlet(), 
                                              interface,
                                              bldmap);

  //  create the inlet wire/event builder
  const int MaxSize = 0x100000;
  const int ebdepth = 4;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *idle->inlet(),
					*outlet, 0, idle->ip(), MaxSize, ebdepth, 0);
  iwire->connect();

  //
  //  attach the servers (EVR and cameras)
  //
  {
    const CamParams& cam1 = *cam_list.begin();
    DetInfo det(node.pid(), 
                (DetInfo::Detector)cam1.detector, cam1.detector_id,
                DetInfo::TM6740, cam1.device_id);
    evrBldMgr = new EvrBldManager(det,evrid);
    evrBldMgr->appliance().connect(idle->inlet());
    iwire->add_input(&(evrBldMgr->server()));
  }

  std::list<PimManager*> cam_managers;
  for(std::list<CamParams>::iterator it=cam_list.begin(); it!=cam_list.end(); it++) {
    DetInfo det(node.pid(), 
                (DetInfo::Detector)it->detector, it->detector_id,
                DetInfo::TM6740, it->device_id);

    PimManager* icamman = new PimManager(det, it->grabber_id);
    icamman->appliance().connect(idle->inlet());
    icamman->attach_camera();
    iwire->add_input(&(icamman->server()));
    cam_managers.push_back(icamman);
  }

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

  for(std::list<PimManager*>::iterator it=cam_managers.begin(); it!=cam_managers.end(); it++) {
    (*it)->detach_camera();
  }

  return 0;
}
