#include "pds/service/CmdLineTools.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/management/PlatformCallback.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/collection/PingReply.hh"
#include "pds/collection/AliasReply.hh"
#include "pds/collection/Route.hh"

#include <string.h>

void showUsage(const char* p)
{
  printf("Usage: %s -p <platform> [-h]\n",p);
}

static void print_ip(int ip)
{
  printf(" %d.%d.%d.%d",
         (ip>>24)&0xff,
         (ip>>16)&0xff,
         (ip>> 8)&0xff,
         (ip>> 0)&0xff);
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

  if (!control.attach()) {
    char buff[64];
    printf("Route is [%s]",Route::name());
    print_ip(Route::interface());
    print_ip(Route::broadcast());
    print_ip(Route::netmask());
    printf(" %s\n",Route::ether().as_string(buff));

    RouteTable table;
    for(unsigned i=0; i<table.routes(); i++) {
      printf("%u [%s]",i,table.name(i));
      print_ip(table.interface(i));
      print_ip(table.broadcast(i));
      print_ip(table.netmask(i));
      printf(" %s\n",table.ether(i).as_string(buff));
    }
    exit(1);
  }

  Observer o;
  control.platform_rollcall(&o);

  sleep(5);

  control.detach();
}
