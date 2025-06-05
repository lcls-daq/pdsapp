#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"

#include "pds/management/SegStreams.hh"
#include "pds/service/Task.hh"
#include "pds/pvdaq/Manager.hh"
#include "pds/pvdaq/Server.hh"

#include "pds/utility/EbS.hh"
#include "pds/utility/EbSequenceKey.hh"
#include "pds/utility/ToEventWireScheduler.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/BitList.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/psddl/bld.ddl.h"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/Level.hh"
#include "pds/service/VmonSourceId.hh"

#include "pds/utility/NullServer.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/utility/InletWireIns.hh"

#include "cadef.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <ifaddrs.h>

static const unsigned MAX_EVENT_SIZE = 4*1024*1024;
static const unsigned MAX_EVENT_DEPTH = 64;
static const unsigned net_buf_depth = 16;
static const unsigned EvrBufferDepth = 32;

#if 0
namespace Pds {

  // remove
  class MyEvrServer : public NullServer {
  public:
    MyEvrServer(const Ins& ins,
                const Src& src,
                unsigned   maxsiz,
                unsigned   maxevt) : NullServer(ins,src,maxsiz,maxevt) {}
  public:
    bool isValued  () const { return true; }
    bool isRequired() const { return true; }
  };

  static MyEvrServer* _evrServer = 0;

  //  Replace with TaggedStreams
  class PvDaqStreams : public WiredStreams {
  public:
    PvDaqStreams(PartitionMember& m,
                 unsigned         max_event_size,
                 unsigned         max_event_depth) :
      WiredStreams(VmonSourceId(m.header().level(), m.header().ip()))
    {
      unsigned max_size = MAX_EVENT_SIZE;

      const Node& node = m.header();
      Level::Type level = node.level();
      int ipaddress = node.ip();
      const Src& src = m.header().procInfo();
      for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

        _outlets[s] = new ToEventWireScheduler(*stream(s)->outlet(),
                                               m,
                                               ipaddress,
                                               max_size*net_buf_depth,
                                               m.occurrences());

        EbS* ebs = new EbS(src,
                           _xtcType,
                           level,
                           *stream(s)->inlet(),
                           *_outlets[s],
                           s,
                           ipaddress,
                           max_event_size, max_event_depth, 0,
                           new VmonEb(src,32,max_event_depth,(1<<23),max_event_size));
        ebs->require_in_order(true);
        ebs->printSinks(false); // these are routine
        _inlet_wires[s] = ebs;

        (new VmonServerAppliance(src))->connect(stream(s)->inlet());
      }
    }
    ~PvDaqStreams() {
      for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
        delete _inlet_wires[s];
        delete _outlets[s];
      }
    }
  };

  //  replace with SegmentLevel
  class PvDaqSegmentLevel : public SegmentLevel {
  public:
    PvDaqSegmentLevel(unsigned     platform,
                      Pds::SegWireSettings& settings,
                      EventCallback& cb) :
      SegmentLevel(platform, settings, cb, 0, 0) {}
    ~PvDaqSegmentLevel() {}
  public:
    bool attach() {
      start();
      if (connect()) {
        _streams = new PvDaqStreams(*this,
                                    _settings.max_event_size(),
                                    _settings.max_event_depth());  // specialized here
        _streams->connect();

        _callback.attached(*_streams);

        //  Add the L1 Data servers
        _settings.connect(*_streams->wire(StreamParams::FrameWork),
                          StreamParams::FrameWork,
                          header().ip());

        //    Message join(Message::Ping);
        //    mcast(join);
        _reply.ready(true);
        mcast(_reply);
        return true;
      } else {
        _callback.failed(EventCallback::PlatformUnavailable);
        return false;
      }
    }
    void allocated(const Allocation& alloc, unsigned index) {
      //  add segment level EVR
      unsigned partition = alloc.partitionid();
      unsigned nnodes    = alloc.nnodes();
      InletWire & inlet  = *_streams->wire(StreamParams::FrameWork);

      for (unsigned n = 0; n < nnodes; n++) {
        const Node & node = *alloc.node(n);
        if (node.level() == Level::Segment &&
	    node == _header) {
	  _contains = node.transient()?_transientXtcType:_xtcType;  // transitions
	  static_cast<EbBase&>(inlet).contains(_contains);  // l1accepts
	}
      }

      for (unsigned n = 0; n < nnodes; n++) {
        const Node & node = *alloc.node(n);
        if (node.level() == Level::Segment) {
          Ins ins = StreamPorts::event(partition, Level::Observer, 0, 0);
          _evrServer =
            new MyEvrServer(ins,
                           header().procInfo(),
                           sizeof(EvrDataType)+256*sizeof(Pds::EvrData::FIFOEvent),
                           EvrBufferDepth);

          Ins mcastIns(ins.address());
          _evrServer->server().join(mcastIns, Ins(header().ip()));

          inlet.add_input(_evrServer);
          _inputs.push_back(_evrServer);
          break;
        }
      }

      unsigned vectorid = 0;

      for (unsigned n = 0; n < nnodes; n++) {
        const Node & node = *alloc.node(n);
        if (node.level() == Level::Event) {
          // Add vectored output clients on inlet
          Ins ins = StreamPorts::event(partition,
                                       Level::Event,
                                       vectorid,
                                       index);
          InletWireIns wireIns(vectorid, ins);
          inlet.add_output(wireIns);
          printf("SegmentLevel::allocated adding output %d to %x/%d\n",
                 vectorid, ins.address(), ins.portId());
          vectorid++;
        }
      }
      OutletWire* owire = _streams->stream(StreamParams::FrameWork)->outlet()->wire();
      owire->bind(OutletWire::Bcast, StreamPorts::bcast(partition,
                                                        Level::Event,
                                                        index));

      //
      //  Assign traffic shaping phase
      //
      const int pid = getpid();
      unsigned s=0;
      for(unsigned n=0; n<nnodes; n++) {
        const Node& node = *alloc.node(n);
        if (node.level()==Level::Segment) {
          if (node.pid()==pid) {
            ToEventWireScheduler::setPhase  (s % vectorid);
            ToEventWireScheduler::setMaximum(vectorid);
            ToEventWireScheduler::shapeTmo  (alloc.options()&Allocation::ShapeTmo);
            break;
          }
          s++;
        }
      }
    }
    void dissolved() {
      _evrServer = 0;
      for(std::list<Server*>::iterator it=_inputs.begin(); it!=_inputs.end(); it++)
        static_cast <InletWireServer*>(_streams->wire())->remove_input(*it);
      _inputs.clear();

      static_cast <InletWireServer*>(_streams->wire())->remove_outputs();
    }
  private:
#ifdef DBUG
    void post(const Transition& tr) {
      printf("Transition %s\n");
      static_cast<InletWireServer*>(_streams->wire(StreamParams::FrameWork))->dump();

      //      SegmentLevel::post(tr);
      if (tr.id()!=TransitionId::L1Accept) {
        _streams->wire(StreamParams::FrameWork)->flush_inputs();
        _streams->wire(StreamParams::FrameWork)->flush_outputs();
      }
      _streams->wire(StreamParams::FrameWork)->post(tr);
    }
#endif

  private:
    std::list<Server*> _inputs;
  };
}
#endif

using namespace Pds;

static void usage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> -b <pvbase>\n"
         "\n"
         "Options:\n"
         "    -i <detinfo>                int/int/int/int or string/int/string/int\n"
         "                                  (e.g. XppEndStation/0/USDUSB/1 or 22/0/26/1)\n"
         "    -b <pvbase>                 base name string of PVs; e.g. MFX:BMMON\n"
         "    -B <pvbase>                 secondary base name string for PVs; e.g. IOC:MFX:BMMON\n"
         "    -p <platform>               platform number\n"
         "    -z <eventsize>              maximum event size[bytes]\n"
         "    -n <eventdepth>             event builder depth\n"
         "    -a <address>                the address to use for connecting to the IOC (useful for bypassing the gateway)\n"
         "    -r                          allow connections to PVs on remote machines\n"
         "    -w <0/1/2>                  set slow readout mode (default: 0)\n"
         "    -h                          print this message and exit\n", p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  const char* pvbase = 0;
  const char* pvbase_alt = 0;
  int slowReadout = 0;
  bool lUsage = false;
  bool local_ioc = true;
  unsigned flags = 0;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NoDetector, 0, DetInfo::NoDevice, 0);
  char* uniqueid = (char *)NULL;
  const char* unicast_addr = NULL;
  unsigned max_event_size = MAX_EVENT_SIZE;
  unsigned max_event_depth = MAX_EVENT_DEPTH;
  char buff[32];

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:b:B:p:u:z:n:f:a:rw:h")) != EOF ) {
    switch(c) {
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,detInfo)) {
        printf("%s: option `-i' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'b':
      pvbase = optarg;
      break;
    case 'B':
      pvbase_alt = optarg;
      break;
    case 'p':
      if (CmdLineTools::parseUInt(optarg,platform) != 1) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'u':
      if (!CmdLineTools::parseSrcAlias(optarg)) {
        printf("%s: option `-u' parsing error\n", argv[0]);
        lUsage = true;
      } else {
        uniqueid = optarg;
      }
      break;
    case 'z':
      max_event_size = strtoul(optarg,NULL,0);
      break;
    case 'n':
      max_event_depth = strtoul(optarg,NULL,0);
      break;
    case 'f':
      if (CmdLineTools::parseUInt(optarg,flags) != 1) {
        printf("%s: option `-f' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'a':
      unicast_addr = optarg;
      break;
    case 'r':
      local_ioc = false;
      break;
    case 'w':
      if (!CmdLineTools::parseInt(optarg,slowReadout)) {
        printf("%s: option `-w' parsing error\n", argv[0]);
        lUsage = true;
      } else if ((slowReadout != 0) && (slowReadout != 1) && (slowReadout != 2)) {
        printf("%s: option `-w' out of range\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'h': // help
      usage(argv[0]);
      return 0;
    case '?':
    default:
      lUsage = true;
      break;
    }
  }

  if (platform == no_entry) {
    printf("%s: platform is required\n", argv[0]);
    lUsage = true;
  }

  if (detInfo.detector() == Pds::DetInfo::NumDetector) {
    printf("%s: detinfo is required\n", argv[0]);
    lUsage = true;
  }

  if (!pvbase) {
    printf("%s: pvbase is required\n", argv[0]);
    lUsage = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usage(argv[0]);
    return 1;
  }

  if(slowReadout==0) {
    printf("Setting normal readout mode for pvdaq process!\n");
  } else {
    printf("Setting slow readout mode for pvdaq process.\n");
  }

  if (unicast_addr) {
    // EPICS_CA_ADDR_LIST only takes numeric addr so we need to resolve any hostnames first
    hostent* entries = gethostbyname(unicast_addr);

    if (entries) {
      char* unicast_ip = inet_ntoa(*(struct in_addr *)entries->h_addr_list[0]);
      printf("Setting EPICS_CA_ADDR_LIST %s\n", unicast_ip);
      setenv("EPICS_CA_ADDR_LIST", unicast_ip, 1);
      setenv("EPICS_CA_AUTO_ADDR_LIST", "NO", 1);
    } else {
      printf("Unable to resolve %s to an IP address!\n", unicast_addr);
    }
  } else if (local_ioc) {
    char dst[64];
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifaddrs *ifap, *ifa;
    sockaddr_in srv;
    memset(&srv,0,sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr   = inet_addr("1.1.1.1");
    srv.sin_port   = htons(53);
    connect(fd, (sockaddr*)&srv, sizeof(srv));
    sockaddr_in name;
    socklen_t   name_sz = sizeof(name);
    getsockname(fd, (sockaddr*)&name, &name_sz);

    // now find the interface for this address to get the broadcast addr
    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET) {
        if (((sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr == name.sin_addr.s_addr) {
          inet_ntop(AF_INET,&((sockaddr_in *) ifa->ifa_broadaddr)->sin_addr,dst,64);
          break;
        }
      }
    }
    freeifaddrs(ifap);

    printf("Setting EPICS_CA_ADDR_LIST %s\n",dst);
    setenv("EPICS_CA_ADDR_LIST",dst,1);
    close(fd);

    setenv("EPICS_CA_AUTO_ADDR_LIST","NO",1);
  }

  sprintf(buff,"%u",max_event_size);
  setenv("EPICS_CA_MAX_ARRAY_BYTES",buff,1);

  //  EPICS thread initialization
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), 
           "pvdaq calling ca_context_create" );

  PvDaq::Server*  server  = PvDaq::Server::lookup(pvbase, pvbase_alt, detInfo, max_event_size, flags);
  PvDaq::Manager* manager = new PvDaq::Manager(*server);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, manager->appliance());
  StdSegWire settings(*server, uniqueid, max_event_size, max_event_depth, false,
                      0,0,true,true);
  //PvDaqSegmentLevel* seglevel = new PvDaqSegmentLevel(platform, settings, *seg);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0, slowReadout);
  seglevel->attach();

  task->mainLoop();

  ca_context_destroy();

  return 0;
}
