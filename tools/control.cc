#include "pds/management/PartitionControl.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/client/Decoder.hh"
#include "pds/collection/PingReply.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <climits>

extern int optind;

namespace Pds {

  class CCallback : public ControlCallback {
  public:
    CCallback() {}
  public:
    void attached(SetOfStreams& streams) {
      Stream& frmk = *streams.stream(StreamParams::FrameWork);
      (new Decoder(Level::Control))   ->connect(frmk.inlet());
    }
    void failed(Reason reason) {}
    void dissolved(const Node& node) {}
  };

  class PCallback : public PlatformCallback {
  public:
    PCallback() {}
  public:
    void clear() { _nnodes = 0; }
    void available(const Node& hdr, const PingReply& msg) {
      printf("Node [0x%08x] %x/%d/%d\n",(1<<_nnodes),hdr.ip(),hdr.level(),hdr.pid());
      if (msg.nsources()) {
  for(unsigned k=0; k<msg.nsources(); k++) {
    const DetInfo& src = static_cast<const DetInfo&>(msg.source(k));
    printf("  %s/%d/%s/%d",
     src.name(src.detector()), src.detId(),
     src.name(src.device()),   src.devId());
  }
  printf("\n");
      }
     _nodes[_nnodes++] = hdr;
    }
    void aliasCollect(const Node& hdr, const AliasReply& msg) {
      // required for PlatformCallback
    }
  public:
    void        select(unsigned m) {  // Note that this overwrites the platform list
      unsigned n=0;
      for(unsigned k=0; k<_nnodes; k++)
  if (m & (1<<k))
    _nodes[n++] = _nodes[k];
      _nnodes = n;

      printf("Partition Selected:\n");
      for(unsigned k=0; k<_nnodes; k++)
  printf(" %x/%d/%d\n",_nodes[k].ip(), _nodes[k].level(), _nodes[k].pid());
    }
    unsigned    nnodes() const { return _nnodes; }
    const Node* nodes () const { return _nodes; }
  private:
    enum { MAX_NODES=32 };
    unsigned _nnodes;
    Node     _nodes[MAX_NODES];
  };
}

using namespace Pds;

static bool parseInt   (const char* arg, int& v, int base=0)
{
  char* endptr;
  v = strtol(arg,&endptr,base);
  return *endptr==0;
}

static bool parseUInt  (const char* arg, unsigned& v, int base=0)
{
  char* endptr;
  v = strtoul(arg,&endptr,base);
  return *endptr==0;
}

int main(int argc, char** argv)
{
  unsigned platform = UINT_MAX;
  unsigned bldList[32];
  unsigned nbld = 0;
  const char* partition = "partition";
  const char* dbpath    = "none";
  const unsigned NO_KEY = UINT_MAX;
  unsigned key = NO_KEY;
  int      slowReadout = 0;
  bool     lusage = false;
  unsigned uu;

  int c;
  while ((c = getopt(argc, argv, "p:b:P:D:k:w:h")) != -1) {
    switch (c) {
    case 'b':
      if (parseUInt(optarg, uu)) {
        bldList[nbld++] = uu;
      } else {
        printf("%s: option `-b' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'p':
      if (!parseUInt(optarg, platform)) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'P':
      partition = optarg;
      break;
    case 'D':
      dbpath = optarg;
      break;
    case 'k':
      if (!parseUInt(optarg, key)) {
        printf("%s: option `-k' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'w':
      if (!parseInt(optarg, slowReadout)) {
        printf("%s: option `-w' parsing error\n", argv[0]);
        lusage = true;
      }
      break;
    case 'h':
      lusage = true;
      break;
    default:
    case '?':
      // error
      lusage = true;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lusage = true;
  }

  if (lusage || (platform==UINT_MAX) || !partition || !dbpath || (key==NO_KEY)) {
    printf("Usage: %s -p <platform> -P <partition_description> -D <db name> -k <key> [-b <bld>]\n", argv[0]);
    return 0;
  }

  CCallback controlcb;
  PCallback platformcb;
  PartitionControl control(platform,
         controlcb, slowReadout);
  /*
  while(nbld--) {
    unsigned id = bldList[nbld];
    Node node(Level::Source, 0);
    node.fixup(Pds::StreamPorts::bld(id).address(),Ether());
    control.add_reporter(node);
  }
  */
  control.attach();
  control.set_transition_env(TransitionId::Configure,key);

  {
    fprintf(stdout, "Commands: EOF=quit\n");
    while (true) {
      printf("Commands (p)ing, (a)llocate [nodes],\n"
       "(u)nmapped, (m)apped, (c)onfigured, (r)unning, (e)nabled,\n");
      const int maxlen=128;
      char line[maxlen];
      char* result = fgets(line, maxlen, stdin);
      if (!result) {
        fprintf(stdout, "\nExiting\n");
        break;
      }
      else {
        int len = strlen(result)-1;
        if (len <= 0) continue;
        while (*result && *result != '\n') {
          char cmd = *result++;
    unsigned env = strtoul(result,&result,16);
    switch(cmd) {
    case 'p':
      platformcb.clear();
      control.platform_rollcall(&platformcb);
      break;
    case 'a':
      if (control.current_state()==PartitionControl::Unmapped) {
        platformcb.select(env);
        control.set_partition(partition,
			      dbpath,
			      "",
			      platformcb.nodes(),
			      platformcb.nnodes(),
			      0, 0, 0);
      }
      else
        printf(" partition already mapped\n");
      break;
    case 'm': control.set_target_state(PartitionControl::Mapped); break;
    case 'c': control.set_target_state(PartitionControl::Configured); break;
    case 'r': control.set_target_state(PartitionControl::Running); break;
    case 'e': control.set_target_state(PartitionControl::Enabled); break;
    case 'u': control.set_target_state(PartitionControl::Unmapped); break;
    default:  printf("Error parsing command %c\n",cmd); break;
    }
        }
      }
    }
  }

  control.detach();

  return 0;
}
