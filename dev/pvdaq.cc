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

static const unsigned MAX_EVENT_SIZE = 8*1024*1024;
static const unsigned eb_depth = 32;
static const unsigned net_buf_depth = 16;
static const unsigned EvrBufferDepth = 32;
static const unsigned AppBufferDepth = eb_depth + 16;  // Handle

namespace Pds {

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

  class PvDaqStreams : public WiredStreams {
  public:
    PvDaqStreams(PartitionMember& m) :
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

        _inlet_wires[s] = new EbS(src,
                                           _xtcType,
                                           level,
                                           *stream(s)->inlet(),
                                           *_outlets[s],
                                           s,
                                           ipaddress,
                                  max_size, eb_depth, 0,
                                           new VmonEb(src,32,eb_depth,(1<<23),(1<<22)));
        
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
        _streams = new PvDaqStreams(*this);  // specialized here
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
      for(unsigned n=0; n<nnodes; n++) {
        const Node& node = *alloc.node(n);
        if (node.level()==Level::Segment)
          if (node.pid()==pid) {
            ToEventWireScheduler::setPhase  (n % vectorid);
            ToEventWireScheduler::setMaximum(vectorid);
            ToEventWireScheduler::shapeTmo  (alloc.options()&Allocation::ShapeTmo);
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

using namespace Pds;

static void usage(const char* p)
{
  printf("Usage: %s -i <detinfo> -p <platform>,<mod>,<chan> -b <pvbase>\n"
         "\n"
         "Options:\n"
         "    -i <detinfo>                int/int/int/int or string/int/string/int\n"
         "                                  (e.g. XppEndStation/0/USDUSB/1 or 22/0/26/1)\n"
         "    -b <pvbase>                 base string of PV name\n"
         "    -p <platform>,<mod>,<chan>  platform number, EVR module, EVR channel\n"
         "    -h                          print this message and exit\n", p);
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const unsigned no_entry = -1U;
  unsigned platform = no_entry;
  unsigned module=0, channel=0;
  const char* pvbase = 0;
  bool lUsage = false;
  Pds::Node node(Level::Source,platform);
  DetInfo detInfo(node.pid(), Pds::DetInfo::NoDetector, 0, DetInfo::NoDevice, 0);
  char* uniqueid = (char *)NULL;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:b:p:u:h")) != EOF ) {
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
    case 'p':
      if (CmdLineTools::parseUInt(optarg,platform,module,channel) != 3) {
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

  //  EPICS thread initialization
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), 
           "pvdaq calling ca_context_create" );

  PvDaq::Server*  server  = PvDaq::Server::lookup(pvbase, detInfo);
  PvDaq::Manager* manager = new PvDaq::Manager(*server);

  Task* task = new Task(Task::MakeThisATask);
  EventAppCallback* seg = new EventAppCallback(task, platform, manager->appliance());
  StdSegWire settings(*server, uniqueid, MAX_EVENT_SIZE);
  PvDaqSegmentLevel* seglevel = new PvDaqSegmentLevel(platform, settings, *seg);
  seglevel->attach();

  task->mainLoop();

  ca_context_destroy();

  return 0;
}
