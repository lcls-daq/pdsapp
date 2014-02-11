#include <signal.h> 
#include <unistd.h>
#include <stdlib.h>  
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "BldIpimbStream.hh"
#include "ToBldEventWire.hh"
#include "EvrBldServer.hh"
#include "EvrBldManager.hh"
#include "BldClient.hh"

#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/SegStreams.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/EventBuilder.hh"

#include "pds/service/Task.hh"
#include "pds/config/CfgClientNfs.hh" 
#include "pds/config/IpimbDataType.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/StreamPorts.hh"

#include "pds/ipimb/IpimbManager.hh"
#include "pds/ipimb/IpimbServer.hh"
#include "pds/ipimb/IpimbFex.hh"
#include "pds/ipimb/LusiDiagFex.hh" 
#include "pds/ipimb/IpimBoard.hh"
#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"

#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Table.hh"
#include "pdsapp/config/EventcodeTiming.hh"

#include "EvrConfigType.hh" 

#define IPIMB_BLD_MAX_DATASIZE  512
#define DEFAULT_EVENT_OPCODE    140 
#define IPIMB_CONFIG_DB         "/reg/g/pcds/dist/pds/sharedIpimb/configdb"
#define IPIMB_PORTMAP_FILE      "/reg/g/pcds/dist/pds/sharedIpimb/bldIpimbPortmap_NH2-SB1-IPM-01.txt"

using namespace Pds;

static EvrBldManager* evrBldMgr = NULL; 

void signalHandler(int sigNo)
{
  printf( "\n signalHandler(): signal %d received.\n",sigNo ); 
  if (evrBldMgr )   {
    evrBldMgr->stop();  
    evrBldMgr->disable();
  }
  exit(0); 
}
  
unsigned parse_interface(const char* iarg)
{
  unsigned interface = 0;
  if (iarg[0]<'0' || iarg[0]>'9') {
    int skt = socket(AF_INET, SOCK_DGRAM, 0);
    if (skt<0) {
      perror("Failed to open socket\n");
      exit(1);
    }
    ifreq ifr;
    strcpy( ifr.ifr_name, iarg);
    if (ioctl( skt, SIOCGIFADDR, (char*)&ifr)==0)
      interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
    else {
      printf("Cannot get IP address for network interface %s.\n",iarg);
      exit(1);
    }
    printf("Using interface %s (%d.%d.%d.%d)\n", iarg,
           (interface>>24)&0xff,
           (interface>>16)&0xff,
           (interface>> 8)&0xff,
           (interface>> 0)&0xff);
    close(skt);
  }
  else {
    in_addr inp;
    if (inet_aton(iarg, &inp))
      interface = ntohl(inp.s_addr);
  }
  return interface;
}

static void showUsage()
{
    printf( "Usage:  bldIpimb [-f <ipimbPortMapFile>] [-c <Ipimb Config DB>] [-i <Interface Name/Ip>] [-o <opcode>] [-h] \n" 
      " Options:\n"
      "   -h                        : Show Usage\n"
      "   -o <eventcode,polarity,delay,width,output> : Configure an EVR output, time in 119MHz clks. \n"
      "   -i <Interface Name/IP>    : Set the network interface for transmitting multicast e.g. eth0, eth1 etc. \n"
      "   -d <DetInfo/bldId>        : Det Info for IPIMB Configuration OR provide Portmap File with -f option. \n"
      "   -c <Config DB Path>       : Path for IPIMB Config DB. \n"
      "   -p <Control Port>         : Control Port for Remote Configuration over Cds Interface. \n"
      "   -b <Baseline Substraction>: Base Substrcation for IPIMB \n"
      "   -r <EVR Id>               : EVR Id e.g. 'a', 'b' etc. Default = 'a' \n"
      "   -f <IPIMB Portmap File>   : Set IPIMB Portmap file to get Detinfo and Bld Id details Default \n");
    
}

static char* addr = new char[100];
char* getBldAddrBase()  
{ 
  unsigned mcastBaseAddr = StreamPorts::bld(0).address();
  sprintf(addr,"%d.%d.%d",(mcastBaseAddr>>24)&0xff,
          (mcastBaseAddr>>16)&0xff, (mcastBaseAddr>> 8)&0xff);
  return addr;
}


int main(int argc, char** argv) {

  unsigned controlPort = 5727; //1100; 
  int interface   = 0; 
 
  std::list<PulseParams> pulses;

  char evrdev[16];  
  char evrid = 'a';
  unsigned bldId = (unsigned) Pds::BldInfo::Nh2Sb1Ipm01; 
  
  int detector   = DetInfo::NoDetector;
  int detectorId = 0;
  int deviceId   = 0;
  int baselineSubtraction = 1;
  unsigned nboards = 1; 
  FILE *fp = NULL;
  char* ipimbConfigDb = new char[100];
  sprintf(ipimbConfigDb,IPIMB_CONFIG_DB);
  

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "o:i:d:c:p:b:r:f:h?")) != EOF ) {
    switch(c) {
    case 'o':
      { PulseParams pulse;
        pulse.eventcode = strtoul(optarg  ,&endPtr,0);
        pulse.polarity  = (strtol (endPtr+1,&endPtr,0) > 0) ? PulseParams::Positive : PulseParams::Negative;
        unsigned ticks = Pds_ConfigDb::EventcodeTiming::timeslot(pulse.eventcode);
        unsigned delta = ticks - Pds_ConfigDb::EventcodeTiming::timeslot(140);
        unsigned udelta = abs(delta);
        pulse.delay     = strtoul(endPtr+1,&endPtr,0);
        if (pulse.delay>udelta) pulse.delay -= delta;
        pulse.width     = strtoul(endPtr+1,&endPtr,0);
        pulse.output    = strtoul(endPtr+1,&endPtr,0);
        pulses.push_back(pulse);
      }
      break;    
    case 'i':
      interface = parse_interface(optarg);
      break;    
    case 'd':
      detector   = (DetInfo::Detector)strtoul(optarg, &endPtr, 0);
      detectorId = strtoul(endPtr+1, &endPtr, 0);
      deviceId   = strtoul(endPtr+1, &endPtr, 0);
      bldId      = strtoul(endPtr+1, &endPtr, 0);
      break;
    case 'c':
      sprintf(ipimbConfigDb,optarg);
      break;
    case 'p':
      controlPort = strtoul(optarg, NULL, 0);
      break;    
    case 'b':
      baselineSubtraction = strtoul(optarg, NULL, 0);
      break;
    case 'r':
      evrid = *optarg;
      break;    
    case 'f':
      fp = fopen(optarg,"r");
      if (fp) 
        printf("Have opened portmap config file %s\n", optarg);
      else {
        printf("Failed to open portmap config file: %s\n", optarg); 
        return 1;
      }
      break;
    case 'h':
    case '?':
      showUsage();
      return 1;
    }
  }

  sprintf(evrdev,"/dev/er%c3",evrid);
  printf("Using evr %s\n",evrdev);
 
  if (bldId >= 255) {
    printf("Invalid Bld Multicast Address: (%s.%d) -Exiting Program \n",getBldAddrBase(),bldId);
    return 1;
  }
  
  if(interface == 0)
    interface = parse_interface("eth0"); // Default interface- eth0

  if(fp == NULL) {
    char portMapFile [] = IPIMB_PORTMAP_FILE;  
    fp = fopen(portMapFile,"r");
    if (fp) 
      printf("Have opened portmap configuration file %s\n", portMapFile);
    else {
      printf("Failed to open ipimb portmap file: %s\n", portMapFile); 
      return 1;
    }
  }
 
  // IPIMB Server
  int polarity = 0;
  char port[16];        
  int portInfo[16][3];  
  char* portName[16];
  for (int i=0; i<16; i++) {
    portName[i] = new char[16];
  }

  int polarities[16];
  unsigned* bldIdMap = new unsigned[16];  
  if (fp) {
    char* tmp = NULL;
    size_t sz = 0;
    nboards = 0;
    while (getline(&tmp, &sz, fp)>0) {
      if (tmp[0] != '#') {
        sscanf(tmp,"%d %d %d %s %d %d",&detector, &detectorId, &deviceId, port, &polarity, &bldId);
        if (bldId >= 255) {
          printf("Invalid Bld Multicast Address: (%s.%d) for Boar BoardNo: %u Exiting Program \n",getBldAddrBase(),bldId,nboards);
          return 1;
        } 
        portInfo[nboards][0] = detector;
        portInfo[nboards][1] = detectorId;
        portInfo[nboards][2] = deviceId;
        strcpy(portName[nboards], port);
        polarities[nboards] = polarity;
        *(bldIdMap+nboards) = bldId;
        nboards++;
      }  
    }
    printf("Have found %d uncommented lines in config file, will use that many boards\n", nboards);
    rewind(fp);
  }
   
  const unsigned nServers = nboards;
  IpimbServer* ipimbServer[nServers];
  CfgClientNfs* cfgService[nServers];
  
  Node node(Level::Source,0);
  for (unsigned i=0; i<nServers; i++) {
    if (!fp) {
      DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detector, detectorId, DetInfo::Ipimb, deviceId);
      cfgService[i] = new CfgClientNfs(detInfo);
      ipimbServer[i] = new IpimbServer(detInfo, false);
      *bldIdMap = bldId;
      portName[i][0] = '\0';
    } else {
      detector = portInfo[i][0];  
      detectorId = portInfo[i][1];
      deviceId = portInfo[i][2];
      DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detector, detectorId, DetInfo::Ipimb, deviceId);
      cfgService[i] = new CfgClientNfs(detInfo);
      ipimbServer[i] = new IpimbServer(detInfo, false);
    }
  }
  if (fp) {
    for (unsigned i=0; i<nServers; i++) {
      printf("Using config file info: detector %d, detector id %d, device id %d, port %s, polarity %d multicast addr:(%s.%d)\n", 
             portInfo[i][0], portInfo[i][1], portInfo[i][2], portName[i], polarities[i],getBldAddrBase(),*(bldIdMap+i));
    }
    fclose(fp);
  } else 
    printf("Using BLD Multicast Addr:(%s.%d)\n",getBldAddrBase(),*bldIdMap);
  
  // Setup ConfigDB and Run Key
  Pds_ConfigDb::Experiment expt(ipimbConfigDb,
                                Pds_ConfigDb::Experiment::NoLock);
  expt.read();
  int runKey = strtoul(expt.table().get_top_entry("BLD")->key().c_str(),NULL,16);
    
  // Setup the Bld Idle stream
  ProcInfo idleSrc(Level::Segment,0,0);
  BldIpimbStream* bldIpimbStream = new BldIpimbStream(controlPort, idleSrc, ipimbConfigDb, runKey);
  printf("$$$ Using ConfigDb: [%s] and RunKey: [%d] \n", ipimbConfigDb,runKey); 

  unsigned maxpayload = IPIMB_BLD_MAX_DATASIZE;
  Ins insDestination = StreamPorts::bld(0); 
  ToBldEventWire* outlet = new ToBldEventWire(*bldIpimbStream->outlet(),interface,maxpayload,insDestination,nServers,bldIdMap);

  // create the inlet wire/event builder
  const int MaxSize = 0x100000;
  const int ebdepth = 4;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *bldIpimbStream->inlet(),
          *outlet, 0, bldIpimbStream->ip(), MaxSize, ebdepth, 0);
  iwire->connect();
  
  // EVR Server 
  DetInfo evrDetInfo(getpid(), DetInfo::NoDetector, 0, DetInfo::Evr, 0);
  EvrBldServer& evrBldServer = *new EvrBldServer(evrDetInfo, *iwire);    
  
  // EVR & IPIMB Mgr Appliances and Stream Connections
  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  evrBldMgr = new EvrBldManager(erInfo, pulses, evrBldServer,cfgService,nServers,bldIdMap);  
  //IpimbManager& ipimbMgr = *new IpimbManager(ipimbServer, nServers, cfgService, portName, baselineSubtraction, polarities, *new IpimbFex);
  IpimbManager& ipimbMgr = *new IpimbManager(ipimbServer, nServers, cfgService, portName, baselineSubtraction, polarities, *new LusiDiagFex);
  evrBldMgr->appliance().connect(bldIpimbStream->inlet());
  ipimbMgr.appliance().connect(bldIpimbStream->inlet());
  
  //  attach the EVR and IPIMB servers
  for(unsigned i=0; i< nServers;i++)
    iwire->add_input(ipimbServer[i]);
  iwire->add_input(&evrBldMgr->server());

  bldIpimbStream->set_inlet_wire(iwire);
  bldIpimbStream->start();


  // Register singal handler 
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = signalHandler; 
  sigActionSettings.sa_flags = 0;
  sigActionSettings.sa_flags   |= SA_RESTART;    
  
  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 ) 
    printf( "main(): Cannot register signal handler for SIGINT\n" );
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 ) 
    printf( "main(): Cannot register signal handler for SIGTERM\n" );
  
  printf("Press Ctrl + C to exit the program...\n" );
  
  /* // Real Time Priority  
  char command[100];
  sprintf(command,"/reg/g/pcds/pds/realTimeProcess/realTime -p %u",getpid()); 
  system (command);  
  */
 Task* task = new Task(Task::MakeThisATask); 
 task->mainLoop();

 
  return 0;
}
