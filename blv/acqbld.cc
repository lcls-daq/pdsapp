#include "pdsapp/blv/ToAcqBldEventWire.hh"
#include "pdsapp/blv/EvrBldServer.hh"
#include "pdsapp/blv/PipeStream.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/Task.hh"
#include "pds/acqiris/AcqFinder.hh"
#include "pds/acqiris/AcqServer.hh"
#include "pds/acqiris/AcqD1Manager.hh"
#include "pds/config/AcqDataType.hh"
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
#include <fcntl.h>

#include <vector>

static int openFifo(const char* name, int flags, unsigned index) {
  char path[128];
  sprintf(path,"/tmp/pimbld_%s_%d",name,index);
  printf("Opening FIFO %s\n",path);
  int result = ::open(path,flags);
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
  std::vector<AcqParams> params;

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
      { AcqParams acq;
        unsigned bld_id      = strtoul(optarg  ,&endPtr,0);
        unsigned detector    = strtoul(endPtr+1,&endPtr,0);
        unsigned detector_id = strtoul(endPtr+1,&endPtr,0);
        unsigned device_id   = strtoul(endPtr+1,&endPtr,0);

        acq.bld_info    = BldInfo( node.pid(), BldInfo::Type(bld_id) );
        acq.det_info    = DetInfo( node.pid(),
                                   DetInfo::Detector(detector), detector_id,
                                   DetInfo::Ipimb, device_id );

        params.push_back(acq);
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
             
  ToAcqBldEventWire* outlet = new ToAcqBldEventWire(*stream->outlet(), 
                                                    interface,
                                                    write_fd,
                                                    params);
  
  //  create the inlet wire/event builder
  const int MaxSize = 0x100000;
  const int ebdepth = 32;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *stream->inlet(),
                                        *outlet, 0, 0x7f000001, MaxSize, ebdepth, 0);
  iwire->connect();

  bool multi_instruments_only = false;
  AcqFinder acqFinder(multi_instruments_only ? 
		      AcqFinder::MultiInstrumentsOnly :
		      AcqFinder::All);

  std::list<AcqD1Manager*> D1Managers;

  Semaphore sem(Semaphore::FULL);
  for(int i=0; i<acqFinder.numD1Instruments();i++) {
    AcqServer* srv = new AcqServer(params[i].det_info,_acqDataType);
    AcqD1Manager* mgr = new AcqD1Manager(acqFinder.D1Id(i),*srv,
                                         *new CfgClientNfs(params[i].det_info),sem);

    iwire->add_input(srv);
    mgr->appliance().connect(stream->inlet());
  }

  iwire->add_input(new EvrBldServer(params[0].det_info, evr_read_fd, *iwire));
  
  stream->set_inlet_wire(iwire);
  stream->start();
  return 0;
}
