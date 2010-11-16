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

#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/SegStreams.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/EventBuilder.hh"

#include "pds/service/Task.hh"
#include "pds/config/CfgClientNfs.hh" 
#include "pds/config/EvrConfigType.hh" 
#include "pds/config/IpimbDataType.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"

#include "pds/ipimb/IpimbManager.hh"
#include "pds/ipimb/IpimbServer.hh"
#include "pds/ipimb/IpimbFex.hh"
#include "pds/ipimb/IpimBoard.hh"
#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"

//pdsdata/xtc/BldInfo.hh & .cc  [enum for Ipimb0 .. Ipimb4] in .hh & names  in .cc
//Id_Ipimb in TypeId .hh and .cc
// Think about #include "IdleControlMsg.hh"  in BldStream
// _msg.key = 0xc;   find a way to locate last saved run-key and then use it  (in BldStream())
// class BldDataIpimb in bldData.hh and cc
//pdsapp/packages add target bldIpimb
//xtcreader.cc as well
//change in bld.cc   



#define IPIMB_BLD_MCAST_ADDRESS_BASE (int)(239<<24 | 255<<16 | 24<<8 | (uint8_t) Pds::BldInfo::Ipimb)   
#define IPIMB_BLD_MCAST_PORT    (unsigned short) 10148
#define IPIMB_BLD_MAX_DATASIZE  512
#define DEFAULT_EVENT_OPCODE    45 
#define IPIMB_CONFIG_DB         "/reg/lab2/home/wilfred/configIpimb/keys"

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
  
unsigned parse_interface(char* iarg)
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
    printf("Using interface %s (%d.%d.%d.%d)\n",
	   iarg,
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


int main(int argc, char** argv) {

  unsigned controlPort = 1100; 
  int interface   = 0x7f000001;
 
  unsigned opcode = DEFAULT_EVENT_OPCODE; 
  char evrdev[16];  
  char evrid = 'a';
  
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
  while ( (c=getopt( argc, argv, "o:i:d:c:p:b:r:f:")) != EOF ) {
    switch(c) {
    case 'o':
      opcode = strtoul(optarg, NULL, 0);
      if(opcode > 255) {
        printf("main()::Invalid Opcode > 255 Selected \n");
        return 1;
      }
      break;	  
    case 'i':
      interface = parse_interface(optarg);
      break;	  
    case 'd':
      detector   = (DetInfo::Detector)strtoul(optarg, &endPtr, 0);
      detectorId = strtoul(endPtr+1, &endPtr, 0);
      deviceId   = strtoul(endPtr+1, &endPtr, 0);
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
        printf("Have opened configuration file %s\n", optarg);
      else {
        printf("failed to open ipimb config file: %s\n", optarg); 
        return 1;
      }
      break;	  
    }
  }

  sprintf(evrdev,"/dev/er%c3",evrid);
  printf("### Using evr %s and opcode = %u \n",evrdev, opcode);  
  
 
  // IPIMB Server
  int polarity = 0;
  char port[16];        
  int portInfo[16][3];  
  char* portName[16];
  for (int i=0; i<16; i++) {
    portName[i] = new char[16];
  }

  int polarities[16];
  unsigned* interfaceOffset = new unsigned[BLD_IPIMB_DEVICES];  
  if (fp) {
    char* tmp = NULL;
    size_t sz = 0;
    nboards = 0;
    while (getline(&tmp, &sz, fp)>0) {
      if (tmp[0] != '#') {
	sscanf(tmp,"%d %d %d %s %d",&detector, &detectorId, &deviceId, port, &polarity);
	portInfo[nboards][0] = detector;
	portInfo[nboards][1] = detectorId;
	portInfo[nboards][2] = deviceId;
	strcpy(portName[nboards], port);
	polarities[nboards] = polarity;
	*(interfaceOffset+nboards) = nboards;
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
      ipimbServer[i] = new IpimbServer(detInfo);
      portName[i][0] = '\0';
    } else {
      detector = portInfo[i][0]; 
      detectorId = portInfo[i][1];
      deviceId = portInfo[i][2];
      DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detector, detectorId, DetInfo::Ipimb, deviceId);
      cfgService[i] = new CfgClientNfs(detInfo);
	  ipimbServer[i] = new IpimbServer(detInfo);
    }
  }
  if (fp) {
    for (unsigned i=0; i<nServers; i++) {
      printf("Using config file info: detector %d, detector id %d, device id %d, port %s, polarity %d\n", 
             portInfo[i][0], portInfo[i][1], portInfo[i][2], portName[i], polarities[i]);
    }
    fclose(fp);
  }
  
  // EVR Server 
  int evrDetid = 0;
  int evrDevid = 0;
  Node evrNode(Level::Source,0);
  DetInfo evrDetInfo(evrNode.pid(), DetInfo::NoDetector, evrDetid, DetInfo::Evr, evrDevid);
  EvrBldServer& evrBldServer = *new EvrBldServer(evrDetInfo);    
  
  // Setup the Bld Idle stream
  ProcInfo idleSrc(Level::Segment,0,0);
  BldIpimbStream* bldIpimbStream = new BldIpimbStream(controlPort, idleSrc, ipimbConfigDb);

  unsigned maxpayload = IPIMB_BLD_MAX_DATASIZE;
  Ins insDestination(IPIMB_BLD_MCAST_ADDRESS_BASE, IPIMB_BLD_MCAST_PORT);
  ToBldEventWire* outlet = new ToBldEventWire(*bldIpimbStream->outlet(),interface,maxpayload,insDestination,nServers,interfaceOffset);

  // EVR & IPIMB Mgr Appliances and Stream Connections
  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  evrBldMgr = new EvrBldManager(erInfo, opcode, evrBldServer,cfgService,nServers,interfaceOffset); 
  IpimbManager& ipimbMgr = *new IpimbManager(ipimbServer, nServers, cfgService, portName, baselineSubtraction, polarities, *new IpimbFex);
  ipimbMgr.appliance().connect(bldIpimbStream->inlet());
  evrBldMgr->appliance().connect(bldIpimbStream->inlet());

  // create the inlet wire/event builder
  const int MaxSize = 0x100000;
  const int ebdepth = 4;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *bldIpimbStream->inlet(),
					*outlet, 0, bldIpimbStream->ip(), MaxSize, ebdepth, 0);
  iwire->connect();
  
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
