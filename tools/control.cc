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



int main(int argc, char** argv)
{
  unsigned platform = 0;
  unsigned bldList[32];
  unsigned nbld = 0;
  const char* partition = "partition";
  const char* dbpath    = "none";
  const unsigned NO_KEY = (unsigned)-1;
  unsigned key = NO_KEY;

  int c;
  while ((c = getopt(argc, argv, "p:b:P:D:k:")) != -1) {
    char* endPtr;
    switch (c) {
    case 'b':
      bldList[nbld++] = strtoul(optarg, &endPtr, 0);
      break;
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = 0;
      break;
    case 'P':
      partition = optarg;
      break;
    case 'D':
      dbpath = optarg;
      break;
    case 'k':
      key = strtoul(optarg, &endPtr, 0);
      break;
    }
  }
  if (!platform || !partition || !dbpath || (key==NO_KEY)) {
    printf("usage: %s -p <platform> -P <partition_description> -D <db name> -k <key> [-b <bld>]\n", argv[0]);
    return 0;
  }

  CCallback controlcb;
  PCallback platformcb;
  PartitionControl control(platform,
			   controlcb);
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
				    platformcb.nodes(),
				    platformcb.nnodes(),
                                    0);
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
