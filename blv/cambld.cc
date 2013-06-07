#include "pdsapp/blv/ToPimBldEventWire.hh"
#include "pdsapp/blv/ToOpalBldEventWire.hh"
#include "pdsapp/blv/EvrBldServer.hh"
#include "pdsapp/blv/PipeStream.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/camera/PimManager.hh"
#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/Opal1kManager.hh"
#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/PicPortCL.hh"
#include "pds/camera/FrameServer.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/Task.hh"

#include "pdsdata/xtc/BldInfo.hh"
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

using namespace Pds;

class CamParams {
public:
  CamParams() : bld_id(0), detector(0), detector_id(0), device_id(0), grabber_id(0), wait_us(0) {}
  unsigned bld_id;
  unsigned detector;
  unsigned detector_id;
  unsigned device;
  unsigned device_id;
  unsigned grabber_id;
  unsigned wait_us;
};

static Pds::CameraDriver* _driver(int id)
{
  return new PdsLeutron::PicPortCL(*new TM6740Camera, id);
}

static void *thread_signals(void*)
{
  while(1) sleep(100);
  return 0;
}

static int openFifo(const char* name, int flags, unsigned index) {
  char path[128];
  sprintf(path,"/tmp/pimbld_%s_%d",name,index);
  printf("Opening FIFO %s\n",path);
  int result = open(path,flags);
  printf("FIFO %s opened.\n",path);
  return result;
}

void usage(const char* p) {
  printf("Usage: %s -i <multicast interface> -c <cam parameters> -n <client index>\n",p);
  printf("\t\"cam parameters\" : bld_id, detector, detector_id, device_id, grabber_id, wait_us\n");
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
  CamParams cam;
  cam.detector    = DetInfo::NoDetector;
  cam.detector_id = 0;
  cam.device      = DetInfo::NoDevice;
  cam.device_id   = 0;

  extern char* optarg;
  char* endPtr;
  int c;
  while ( (c=getopt( argc, argv, "i:c:n:")) != EOF ) {
    switch(c) {
    case 'i':
      interface = parse_network(optarg);
      break;
    case 'c':
      {
        cam.bld_id      = strtoul(optarg  ,&endPtr,0);
        DetInfo info(endPtr+1);
        if (info.detector() != DetInfo::NumDetector) {
          cam.detector    = info.detector();
          cam.detector_id = info.detId();
          cam.device      = info.device();
          cam.device_id   = info.devId();
          endPtr += strlen(DetInfo::name(info))+1;
        }
        else {
          cam.detector    = strtoul(endPtr+1,&endPtr,0);
          cam.detector_id = strtoul(endPtr+1,&endPtr,0);
          cam.device      = strtoul(endPtr+1,&endPtr,0);
          cam.device_id   = strtoul(endPtr+1,&endPtr,0);
        }
        cam.grabber_id  = strtoul(endPtr+1,&endPtr,0);
        cam.wait_us     = strtoul(endPtr+1,&endPtr,0);
        
        printf("Parsed cam parameters bld %s  cam %s  grabber %d  delay %d\n",
               BldInfo::name(BldInfo(0,BldInfo::Type(cam.bld_id))),
               DetInfo::name(DetInfo(0,DetInfo::Detector(cam.detector),
                                     cam.detector_id,
                                     DetInfo::Device(cam.device),
                                     cam.device_id)),
               cam.grabber_id,
               cam.wait_us);

      } break;
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
  
  printf("Starting handler thread ...\n");
  pthread_t h_thread_signals;
  pthread_create(&h_thread_signals, NULL, thread_signals, 0);
      
  //
  // Block all signals from this thread (and daughters)
  //
  sigset_t sigset_full;
  sigfillset(&sigset_full);
  pthread_sigmask(SIG_BLOCK, &sigset_full, 0);

  Node node(Level::Source,0);
  ProcInfo idleSrc(Level::Segment,0,0);
  PipeStream* stream = new PipeStream(idleSrc, read_fd);
             
  BldInfo bld(node.pid(),(BldInfo::Type)cam.bld_id);
  ToBldEventWire* outlet;
  switch(bld.type()) {
  case BldInfo::CxiDg3Spec:
    outlet = new ToOpalBldEventWire(*stream->outlet(), 
                                    interface,
                                    write_fd,
                                    bld,
                                    cam.wait_us);
    break;
  default:
    outlet = new ToPimBldEventWire(*stream->outlet(), 
                                   interface,
                                   write_fd,
                                   bld,
                                   cam.wait_us);
    break;
  }
    
  //  create the inlet wire/event builder
  const int MaxSize = 0x900000;
  const int ebdepth = 32;
  InletWire* iwire = new L1EventBuilder(idleSrc, _xtcType, Level::Segment, *stream->inlet(),
                                        *outlet, 0, 0x7f000001, MaxSize, ebdepth, 0);
  iwire->connect();
  
  DetInfo det(node.pid(), 
              (DetInfo::Detector)cam.detector, cam.detector_id,
              (DetInfo::Device)cam.device, cam.device_id);

  if (cam.bld_id) {
    switch(det.device()) {
    case DetInfo::TM6740:
      { PimManager* icamman = new PimManager(det);
        icamman->appliance().connect(stream->inlet());
        icamman->attach(new PdsLeutron::PicPortCL(*new TM6740Camera, 
                                                  cam.grabber_id));
        iwire->add_input(&(icamman->server()));
      } break;
    case DetInfo::Opal1000:
      { Opal1kManager* icamman = new Opal1kManager(det);
        icamman->appliance().connect(stream->inlet());
        icamman->attach(new PdsLeutron::PicPortCL(*new Opal1kCamera(det), 
                                                  cam.grabber_id));
        iwire->add_input(&(icamman->server()));
      } break;
    default:
      break;
    }
  }

  iwire->add_input(new EvrBldServer(det, evr_read_fd, *iwire));
  
  stream->set_inlet_wire(iwire);
  stream->start();
  return 0;
}
