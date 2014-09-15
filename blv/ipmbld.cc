#include "pdsapp/blv/ToIpmBldEventWire.hh"
#include "pdsapp/blv/EvrBldServer.hh"
#include "pdsapp/blv/PipeStream.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/Task.hh"
#include "pds/ipimb/IpimbServer.hh"
#include "pds/ipimb/IpimbManager.hh"
#include "pds/ipimb/LusiDiagFex.hh"
#include "pds/config/CfgClientNfs.hh"

#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <vector>

static int openFifo(const char* name, int flags, unsigned index) {
  char path[128];
  sprintf(path,"/tmp/pimbld_%s_%d",name,index);
  printf("Opening FIFO %s\n",path);
  int result = open(path,flags);
  printf("FIFO %s opened.\n",path);
  return result;
}

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -i <multicast interface> -p <ipm parameters> -n <client index>\n",p);
  printf("\t\"ipm parameters\" : bld_id, detector, detector_id, device_id, serial_port_name \n");
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
  unsigned client(0);
  std::vector<IpmParams> ipms;
  int baselineSubtraction = 1;

  Node node(Level::Source,0);

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "i:p:n:")) != EOF ) {
    switch(c) {
    case 'i':
      interface = parse_network(optarg);
      break;
    case 'p':
      { IpmParams ipm;
        unsigned bld_id      = strtoul(optarg  ,&endPtr,0);
        unsigned detector    = strtoul(endPtr+1,&endPtr,0);
        unsigned detector_id = strtoul(endPtr+1,&endPtr,0);
        unsigned device_id   = strtoul(endPtr+1,&endPtr,0);

        ipm.bld_info    = BldInfo( node.pid(), BldInfo::Type(bld_id) );
        ipm.det_info    = DetInfo( node.pid(),
                                   DetInfo::Detector(detector), detector_id,
                                   DetInfo::Ipimb, device_id );
        ipm.port_name   = std::string(endPtr+1);

        ipms.push_back(ipm);
        break; }
    case 'n':
      client = strtoul(optarg,&endPtr,0);
      break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  if (interface==0) {
    usage(argv[0]);
    exit(1);
  }

  int read_fd     = openFifo("TransInputQ",O_RDONLY,client);
  int write_fd    = openFifo("TransInputQ",O_WRONLY,client+1);
  int evr_read_fd = openFifo("EventInputQ",O_RDONLY,client);
  
  ProcInfo idleSrc(Level::Segment,0,0);
  PipeStream* stream = new PipeStream(idleSrc, read_fd);
             
  ToIpmBldEventWire* outlet = new ToIpmBldEventWire(*stream->outlet(), 
                                                    interface,
                                                    write_fd,
                                                    ipms);
  
  //  create the inlet wire/event builder
  const int MaxSize = 0x100000;
  const int ebdepth = 32;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *stream->inlet(),
                                        *outlet, 0, 0x7f000001, MaxSize, ebdepth, 0);
  iwire->connect();

  const unsigned nServers = ipms.size();
  IpimbServer**  ipimbServer = new IpimbServer* [nServers];
  CfgClientNfs** cfgService  = new CfgClientNfs*[nServers];
  int*           polarities  = new int[nServers];
  char**         portNames   = new char*[nServers];

  for (unsigned i=0; i<nServers; i++) {
    cfgService [i] = new CfgClientNfs(ipms[i].det_info);
    ipimbServer[i] = new IpimbServer (ipms[i].det_info, false);
    polarities [i] = 0;
    portNames  [i] = new char[ipms[i].port_name.size()+1];
    strcpy(portNames[i],ipms[i].port_name.c_str());
  }

  IpimbManager& ipimbMgr = *new IpimbManager(ipimbServer, 
                                             nServers, 
                                             cfgService, 
                                             portNames, 
                                             baselineSubtraction, 
                                             polarities, 
                                             *new LusiDiagFex);
  ipimbMgr.appliance().connect(stream->inlet());


  for (unsigned i=0; i<nServers; i++)
    iwire->add_input(ipimbServer[i]);

  DetInfo evr_info(0,DetInfo::NoDetector,0,DetInfo::Evr,0);
  iwire->add_input(new EvrBldServer(evr_info, evr_read_fd, *stream->inlet()));
  
  stream->set_inlet_wire(iwire);
  stream->start();
  return 0;
}
