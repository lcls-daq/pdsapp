#include "pdsapp/blv/ToIpmBldEventWire.hh"
#include "pdsapp/blv/EvrBldServer.hh"
#include "pdsapp/blv/PipeStream.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/Task.hh"

#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pdsdata/ipimb/DataV2.hh"

#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <math.h>
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

static int ndrop = 0;
static int ntime = 0;
static double ftime = 0;

static int openFifo(const char* name, int flags, unsigned index) {
  char path[128];
  sprintf(path,"/tmp/pimbld_%s_%d",name,index);
  printf("Opening FIFO %s\n",path);
  int result = open(path,flags);
  printf("FIFO %s opened.\n",path);
  return result;
}

class SimApp : public Appliance {
public:
  SimApp(const std::vector<IpmParams>& ipms) :
    _cfgtc    (ipms.size()),
    _config   (ipms.size()),
    _fexcfgtc (ipms.size()),
    _fexconfig(ipms.size()),
    _evttc    (ipms.size()),
    _data     (ipms.size()),
    _fexevttc (ipms.size()),
    _fexdata  (ipms.size())
  {
    for(unsigned i=0; i<ipms.size(); i++) {

      // Configure data
      _cfgtc[i] = Xtc(_ipimbConfigType, ipms[i].det_info);
      _cfgtc[i].extent += sizeof(IpimbConfigType);
      memset(&_config[i],0,sizeof(IpimbConfigType));

      _fexcfgtc[i] = Xtc(_ipmFexConfigType, ipms[i].det_info);
      _fexcfgtc[i].extent += sizeof(IpmFexConfigType);
      memset(&_fexconfig[i],0,sizeof(IpmFexConfigType));

      // Event data
      _evttc[i] = Xtc(TypeId(TypeId::Id_IpimbData,2), ipms[i].det_info);
      _evttc[i].extent += sizeof(Pds::Ipimb::DataV2);

      *reinterpret_cast<uint64_t*>(&_data[i]) = 0;    // triggerCounter

      uint16_t* v = reinterpret_cast<uint16_t*>(&_data[i])+7;
      for(unsigned j=0; j<4; j++)
	v[j] = 0x1000;

      _fexevttc[i] = Xtc(TypeId(TypeId::Id_IpmFex,1), ipms[i].det_info);
      _fexevttc[i].extent += sizeof(Pds::Lusi::IpmFexV1);

      float* p = reinterpret_cast<float*>(&_fexdata[i]);
      float sum = 0;
      for(unsigned j=0; j<4; j++)
	sum += (p[j] = 0.2+0.1*float(j));
      p[4] = sum;
      p[5] = (p[2]-p[0])/(p[2]+p[0]);
      p[6] = (p[3]-p[1])/(p[3]+p[1]);
    }
  }
  ~SimApp() {}
public:
  Transition* transitions(Transition* tr) 
  { 
    return tr; 
  }
  InDatagram* events     (InDatagram* dg) 
  {
    switch(dg->seq.service()) {
    case TransitionId::Configure:
      for(unsigned i=0; i<_cfgtc.size(); i++) {
	dg->insert(_cfgtc   [i], &_config   [i]);
	dg->insert(_fexcfgtc[i], &_fexconfig[i]);
      }
      break;
    case TransitionId::L1Accept:
    {
      static int itime=0;
      if ((++itime)==ntime) {
	itime = 0;
	timeval ts = { int(ftime), int(drem(ftime,1)*1000000) };
	select( 0, NULL, NULL, NULL, &ts);
      }

      static int idrop=0;
      if (++idrop==ndrop) {
	idrop=0;
	return 0;
      }

      for(unsigned i=0; i<_evttc.size(); i++) {
	//  generate non-trivial data
	reinterpret_cast<uint64_t&>(_data[i])++;    // triggerCounter

	uint16_t* v = reinterpret_cast<uint16_t*>(&_data[i])+7;
	for(unsigned j=0; j<4; j++)
	  v[j] = 0x7fff-v[j];

	float* p = reinterpret_cast<float*>(&_fexdata[i]);
	float sum = 0;
	for(unsigned j=0; j<4; j++)
	  sum += (p[j] = (1-p[j]));
	p[4] = sum;
	p[5] = (p[2]-p[0])/(p[2]+p[0]);
	p[6] = (p[3]-p[1])/(p[3]+p[1]);

	dg->insert(_evttc   [i], &_data   [i]);
	dg->insert(_fexevttc[i], &_fexdata[i]);
      }
      break;
    }
    default:
      break;
    }
    return dg; 
  }
private:
  std::vector<Xtc> _cfgtc;
  std::vector<IpimbConfigType> _config;
  std::vector<Xtc> _fexcfgtc;
  std::vector<IpmFexConfigType> _fexconfig;
  std::vector<Xtc> _evttc;
  std::vector<Ipimb::DataV2> _data;
  std::vector<Xtc> _fexevttc;
  std::vector<Lusi::IpmFexV1> _fexdata;
};

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -i <multicast interface> -p <ipm parameters> -n <client index>\n"
	 "       -D <N> Drop every N events\n"
	 "       -T <S>,<N>  Delay S seconds every N events\n",
	 p);
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
  while ( (c=getopt( argc, argv, "i:p:n:D:T:h")) != EOF ) {
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
    case 'D':
      ndrop = strtoul(optarg, NULL, 0);
      break;
    case 'T':
      ntime = strtoul(optarg, &endPtr, 0);
      ftime = strtod (endPtr+1, &endPtr);
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

  DetInfo evr_info(0,DetInfo::NoDetector,0,DetInfo::Evr,0);
  iwire->add_input(new EvrBldServer(evr_info, evr_read_fd, *iwire));
  
  stream->set_inlet_wire(iwire);

  (new SimApp(ipms))->connect(stream->inlet());

  stream->start();
  return 0;
}
