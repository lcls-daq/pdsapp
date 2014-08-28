#include "pds/service/CmdLineTools.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/collection/PingReply.hh"
#include "pds/collection/AliasReply.hh"

#include <string.h>

void showUsage(const char* p)
{
  printf("Usage: %s -p <platform> [-h]\n",p);
}

namespace Pds {
  class Callback : public ControlCallback {
  public:
    Callback() {}
    ~Callback() {}
  public:
    void attached (SetOfStreams&)  { printf("attached\n");  }
    void failed   (Reason reason) { printf("failed\n");    }
    void dissolved(const Node&)   { printf("dissolved\n"); }
  };

  class Observer : public PlatformCallback {
  public:
    Observer() {}
    ~Observer() {}
  public:
    void available   ( const Node& node, const PingReply&  reply) {
      printf("Node: %s %d %08x\n",
             Level::name(node.level()),
             node.pid(),
             node.ip());
      for(unsigned i=0; i<reply.nsources(); i++) {
        const Src& src = reply.source(i);
        printf("\t%08x.%08x\n",src.log(),src.phy());
      }
    }
    void aliasCollect( const Node& node, const AliasReply& reply) {
      printf("Alias: %s %d %08x\n",
             Level::name(node.level()),
             node.pid(),
             node.ip());
      for(unsigned i=0; i<reply.naliases(); i++) {
        const SrcAlias& a = reply.alias(i);
        printf("\t%08x.%08x\t%s\n",a.src().log(),a.src().phy(),a.aliasName());
      }
    }
  };
};

using namespace Pds;

int main(int argc, char* argv [])
{
  bool parseErr = false;
  bool platformSet = false;
  unsigned platform=0;
  extern int optind;

  int c;
  while ((c = getopt(argc, argv, "p:h")) != -1) {
    switch (c) {
    case 'p':
      platformSet = true;
      if (!CmdLineTools::parseUInt(optarg, platform)) {
        parseErr = true;
      }
      break;
    case 'h':
    case '?':
    default:
      parseErr = true;
      break;
    }
  }

  if (!platformSet) {
    printf("%s: platform is required\n", argv[0]);
    parseErr = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    parseErr = true;
  }

  if (parseErr) {
    showUsage(argv[0]);
    exit(1);
  }

  Callback cb;
  PartitionControl control(platform, cb, 0);
  control.attach();

  Observer o;
  control.platform_rollcall(&o);

  sleep(5);

  control.detach();
}
